#include "../manager.h"
#include "../algorithm/state.h"
#include "../common.h"
#include "abstract_spot_market_strategy.h"


#include <cassert>
#include <iostream>
#include <limits>

const ui32 MAX_INDEX = std::numeric_limits<ui32>::max();
const double INF = std::numeric_limits<double>::max();

namespace sea {

void toNextEvent(TimeParameters* parameters) {
    parameters->gotPortDecision = false;
    parameters->doneDecision = false;
    parameters->doneAction = false;
    ++parameters->timeEvent;
}

namespace strategy {

using EventType=InputData::Event::Type;

AbstractSpotMarketStrategy::AbstractSpotMarketStrategy(
        const SpotMarketStrategyConfig& aConfig,
        const BackendConfigHolder& aBackendConfigs,
        const std::string aName)
    : backendConfigs(aBackendConfigs)
    , config(aConfig)
    , name(aName) {
    initialize();
    logging::getSpotStrategyLogger(config.type.value()).info(
            "Created AbstractSpotMarketStrategy.");
}

ConstDecisionManagerPtr AbstractSpotMarketStrategy::makeDecision() {
    logging::getSpotStrategyLogger(config.type.value()).debug(
            "AbstractSpotMarketStrategy::makeDecision is called.");

    auto* decision = decisionManager->get();
    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();
    auto& timeParameters = state.timeParameters;
    decision->time = state.timeParameters.timeEvent;
    assert(timeParameters.allotmentsSupplied);
    assert(input.events.size() > timeParameters.timeEvent);
    const auto eventNow = input.events[timeParameters.timeEvent];
    if (eventNow.type == EventType::pricing) {
        if (!timeParameters.doneDecision) {
            assert(!timeParameters.doneAction);
            assert(!timeParameters.gotPortDecision);
            processPricing(eventNow);
            timeParameters.doneDecision = true;
        } else {
            assert(timeParameters.doneAction);
            assert(!timeParameters.gotPortDecision);
            processOffhiring(eventNow);
            timeParameters.gotPortDecision = true;
            toNextEvent(&timeParameters);
        }
    } else if (eventNow.type == EventType::cutoff) {
        if (!timeParameters.doneDecision) {
            assert(timeParameters.doneAction);
            assert(!timeParameters.gotPortDecision);
            processCutoff(eventNow);
            timeParameters.doneDecision = true;
            // update state
            for (ui32 routeId : eventNow.relatedItineraryIds) {
                assert(state.takenOnRoute[routeId] == 0);
                state.takenOnRoute[routeId] += decision->nonEmptyContainersQ[routeId];
                state.takenOnRoute[routeId] += decision->emptyContainersZ[routeId];
                for (ui32 contractId = 0; contractId < input.allotments.size(); ++contractId) {
                    if (links.allotmentItineraryToPlace[contractId].find(routeId) !=
                            links.allotmentItineraryToPlace[contractId].end()) {
                        ui32 placeIndex = links.allotmentItineraryToPlace[contractId].at(routeId);
                        assert(placeIndex != MAX_INDEX);
                        assert(routeId == decision->allotmentContainersQ[contractId][placeIndex].first);
                        auto Q = decision->allotmentContainersQ[contractId][placeIndex].second;
                        state.takenOnRoute[routeId] += Q;
                    }
                }
            }
        } else {
            assert(timeParameters.doneAction);
            assert(!timeParameters.gotPortDecision);
            processOffhiring(eventNow);
            timeParameters.gotPortDecision = true;
            toNextEvent(&timeParameters);
        }
    } else if (eventNow.type == EventType::arrival) {
        assert(!timeParameters.doneAction);
        assert(!timeParameters.doneDecision);
        assert(!timeParameters.gotPortDecision);
        processArrival(eventNow);
        processOffhiring(eventNow);
        timeParameters.gotPortDecision = true;
        toNextEvent(&timeParameters);
    } else {
        throw std::logic_error("This event type is not supported");
    }
    logging::getSpotStrategyLogger(config.type.value()).debug(
            "AbstractSpotMarketStrategy::makeDecision finished.");
    return decisionManager;
}

void AbstractSpotMarketStrategy::supplyAllotmentDecision(
        const DecisionManagerPtr& decisionManagerNew) {
    decisionManager = decisionManagerNew;
    initState(config.inputManager->getConstData(), &state);
    state.timeParameters.allotmentsSupplied = true;
}

void AbstractSpotMarketStrategy::submitAction(ConstActionManagerPtr newActionManager) {
    actionManager = newActionManager;
    auto* action = &newActionManager->getConstData();
    auto& timeParameters = state.timeParameters;
    const auto& input = config.inputManager->getConstData();
    assert(action->time >= timeParameters.timeEvent);
    assert(timeParameters.allotmentsSupplied);
    assert(input.events.size() > timeParameters.timeEvent);
    const auto& eventNow = input.events[timeParameters.timeEvent];
    assert(eventNow.type != EventType::arrival);
    assert(action->time == timeParameters.timeEvent);
    if (eventNow.type == EventType::cutoff) {
        assert(!timeParameters.doneDecision);
        assert(!timeParameters.doneAction);
        assert(!timeParameters.gotPortDecision);
        timeParameters.doneAction = true;
    } else if (eventNow.type == EventType::pricing) {
        assert(timeParameters.doneDecision);
        assert(!timeParameters.doneAction);
        assert(!timeParameters.gotPortDecision);
        timeParameters.doneAction = true;
        // update bookings in state
        for (ui32 i = 0; i < action->bookingsB[eventNow.relativeTime].size(); ++i) {
            state.accumulatedBookings[i] += action->bookingsB[eventNow.relativeTime][i];
        }
    } else {
        throw std::logic_error("This event type is not supported");
    }
}

void AbstractSpotMarketStrategy::processPricing(const InputData::Event& event) {
    updateParams(event);
}

void AbstractSpotMarketStrategy::processArrival(const InputData::Event& event) {
    updateParams(event);
    const auto& input = config.inputManager->getConstData();
    assert(event.type == EventType::arrival);
    const auto& arc = input.arcs[event.basedArc.value()];
    const auto& node = input.nodes[arc.toNode];
    const auto& port = input.ports[node.portId];
    auto* decision = &decisionManager->getConstData();
    auto& containersInPorts = state.containersInPorts;
    const auto& links = config.linksManager->getConstData();
    for (ui32 itineraryId : event.relatedItineraryIds) {
        containersInPorts[port.id] += decision->emptyContainersZ[itineraryId];
        containersInPorts[port.id] += decision->nonEmptyContainersQ[itineraryId];
        for (ui32 allotmentId : links.allotmentsWithItinerary[itineraryId]) {
            if (decision->allotmentAccepted[allotmentId]) {
                ui32 place = links.allotmentItineraryToPlace[allotmentId].at(itineraryId);
                containersInPorts[port.id] += decision->allotmentContainersQ[allotmentId][place].second;
            }
        }
    }

}

void AbstractSpotMarketStrategy::processOffhiring(const InputData::Event& event) {
    updateParams(event);
    ui32 eventTime = event.relativeTime;
    auto* decision = decisionManager->get();
    auto& containersInPorts = state.containersInPorts;
    const auto& input = config.inputManager->getConstData();
    for (ui32 idPort = 0; idPort < input.ports.size(); ++idPort)  {
        if (int(decision->offHiredInPortS[eventTime][idPort]) > containersInPorts[idPort]) {
            decision->offHiredInPortS[eventTime][idPort] = containersInPorts[idPort];
        }
        if (event.relativeTime + 1 == input.events.size()) {
            if (containersInPorts[idPort] >= int(input.ports[idPort].finalContainerCount)) {
                decision->offHiredInPortS[eventTime][idPort] =
                    containersInPorts[idPort] - input.ports[idPort].finalContainerCount;
            }
        }
        containersInPorts[idPort] -= decision->offHiredInPortS[eventTime][idPort];
        assert(containersInPorts[idPort] >= 0);
    }
}

void AbstractSpotMarketStrategy::initialize() {
    lastUpdate = -1e100;
    story.clear();
    state = State();
    initState(config.inputManager->getConstData(), &state);
    const auto& events = config.inputManager->getConstData().events;
    if (events.size() && config.moveUpdateInterval) {
        lastUpdate = events[0].realTime;
    }
    decisionManager = nullptr;
    actionManager = nullptr;
}

void AbstractSpotMarketStrategy::hardReset() {
    backends.dcpBackend = nullptr;
    backends.lrBackend = nullptr;
    backends.ipoptBackend = nullptr;
    backends.bendersBackend = nullptr;
    reset();
}

} // namespace strategy
} // namespace sea
