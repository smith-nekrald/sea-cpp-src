// Implements logic for Ipopt Spot Market Strategy.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023


#include "ipopt_spot_strategy.h"
#include "../../logging/logging.h"

namespace sea {
namespace strategy {

void IpoptSpotMarketStrategy::updateParams(const InputData::Event& event) {
    bool needUpdate = (event.realTime - lastUpdate) > config.updateInterval;
    logEnteredUpdateParams(needUpdate);
    if (needUpdate) {
        double futureValue = 0;
        backends.ipoptBackend->moveDecisionToTime(
                state.timeParameters,
                decisionManager,
                actionManager,
                &futureValue);
        lastUpdate = event.realTime;
        if (keepStory) {
            story["event_time"].push_back(event.realTime);
            story["ipopt_future_value"].push_back(futureValue);
        }
    }
    logLeftUpdateParams();
}

void IpoptSpotMarketStrategy::processCutoff(const InputData::Event& event) {
    updateParams(event);

    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();

    auto* decision = decisionManager->get();
    auto* action = &actionManager->getConstData();

    const auto& arc = input.arcs[event.basedArc.value()];

    unsigned portId = input.nodes[arc.fromNode].portId;
    for (unsigned idItinerary : event.relatedItineraryIds) {
        // Processing Q_r
        unsigned shownCountSpot = action->spotMarketDemandN[idItinerary];
        auto& Q_value = decision->nonEmptyContainersQ[idItinerary];
        Q_value = std::min(Q_value, shownCountSpot);
        state.containersInPorts[portId] -= Q_value;

        auto& Z_value = decision->emptyContainersZ[idItinerary];
        state.containersInPorts[portId] -= Z_value;

        // Processing allotments
        for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
            if (decision->allotmentAccepted[idAllotment]) {
                unsigned placeIndex = links.allotmentItineraryToPlace.at(
                        idAllotment).at(idItinerary);
                unsigned shownAllotment = action->allotmentDemandN[idAllotment][placeIndex].second;
                assert(action->allotmentDemandN[idAllotment][placeIndex].first == idItinerary);
                assert(decision->allotmentContainersQ[idAllotment][placeIndex].first
                        == idItinerary);
                auto& QI_value = decision->allotmentContainersQ[idAllotment][placeIndex].second;
                QI_value = std::min(QI_value, shownAllotment);
                state.containersInPorts[portId] -= QI_value;
            }
        }
    }
    state.containersInPorts[portId] += decision->hiredY[arc.id];
    if (state.containersInPorts[portId] < 0) {
        decision->hiredY[arc.id] -= state.containersInPorts[portId];
        state.containersInPorts[portId] = 0;
    }
}

void IpoptSpotMarketStrategy::reset() {
    initialize();
    initBackend(backends.ipoptBackend,
            backendConfigs.ipoptConfig,
            config.utilizationRatio.value());
    backends.dcpBackend = nullptr;
    backends.bendersBackend = nullptr;
    backends.lrBackend = nullptr;
}

void IpoptSpotMarketStrategy::setBackends(const BackendHolder& holder) {
    if (holder.ipoptBackend != nullptr) {
        backends.ipoptBackend = holder.ipoptBackend;
        backends.ipoptBackend->setUtilizationRatio(config.utilizationRatio.value());
    }
    backends.dcpBackend = nullptr;
    backends.lrBackend = nullptr;
    backends.bendersBackend = nullptr;
}

} // namespace strategy
} // namespace sea
