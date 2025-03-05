// Implements logic for LR Spot Market Strategy.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
// (c) Smith School of Business, 2025

#include <limits>

#include "lr_spot_market_strategy.h"

namespace sea {
namespace strategy {

using Event=InputData::Event;

void LRBasedSpotMarketStrategy::updateParams(const Event& event) {
    bool needUpdate = (event.realTime - lastUpdate) > config.updateInterval;
    bool needSoftUpdate = (event.realTime - lastSoftUpdateTime) > softUpdatePeriod;
    if (needUpdate) {
        lastUpdate = event.realTime;
        if (doHardUpdate) {
            if (askIpoptDuals) {
                backends.ipoptBackend->moveDecisionToTime(
                    state.timeParameters, decisionManager,
                    actionManager, nullptr, &ipoptDuals);
            } else {
                backends.ipoptBackend->moveDecisionToTime(
                    state.timeParameters, decisionManager,
                    actionManager, nullptr, nullptr);
            }
        }
    }
    if (needUpdate || needSoftUpdate) {
        lastSoftUpdateTime = event.realTime;
        if (askIpoptDuals && ipoptDuals.muVariables.size() && ipoptDuals.lambdaVariables.size()) {
            backends.lrBackend->provideIpoptDuals(ipoptDuals);
        }
        double futureValue = 0.;
        duals = backends.lrBackend->provideDuals(state, decisionManager, nullptr, &futureValue);
        if (keepStory) {
            story["event_time"].push_back(event.realTime);
            story["future_value"].push_back(futureValue);
        }
        logDualsInUpdateParams(event);
    }
}

void LRBasedSpotMarketStrategy::processCutoff(const Event& event) {
    updateParams(event);
    backends.lrBackend->makeCutoffDecisionWithDuals(
            event, duals, actionManager, decisionManager, &state);
}

void LRBasedSpotMarketStrategy::reset() {
    initialize();
    initBackend(backends.lrBackend, backendConfigs.lrConfig);
    initBackend(
            backends.ipoptBackend, backendConfigs.ipoptConfig, config.utilizationRatio.value());
    backends.bendersBackend = nullptr;
    const double INF = std::numeric_limits<double>::max();
    lastUpdate = -INF;
    lastSoftUpdateTime = -INF;
}

void LRBasedSpotMarketStrategy::setBackends(const BackendHolder& holder)  {
    if (holder.ipoptBackend != nullptr) {
        backends.ipoptBackend = holder.ipoptBackend;
        backends.ipoptBackend->setUtilizationRatio(config.utilizationRatio.value());
    }
    if (holder.lrBackend != nullptr) {
        backends.lrBackend = holder.lrBackend;
    }
    backends.bendersBackend = nullptr;
}

} // namespace strategy
} // namespace sea
