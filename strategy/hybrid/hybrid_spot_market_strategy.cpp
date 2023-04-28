#include "hybrid_spot_market_strategy.h"

namespace sea {
namespace strategy {

void HybridSpotMarketStrategy::updateParams(const InputData::Event& event) {
    bool needUpdate = (event.realTime - lastUpdate) > config.updateInterval;
    if (needUpdate) {
        assert(state.timeParameters.allotmentsSupplied);
        double futureValue = 0.;
        backends.ipoptBackend->moveDecisionToTime(
                state.timeParameters, decisionManager, actionManager, &futureValue, &duals);
        lastUpdate = event.realTime;
        if (keepStory) {
            story["event_time"].push_back(event.realTime);
            story["ipopt_future_value"].push_back(futureValue);
        }
    }
}

void HybridSpotMarketStrategy::processCutoff(const InputData::Event& event) {
    updateParams(event);
    backends.lrBackend->makeCutoffDecisionWithDuals(
            event, duals, actionManager, decisionManager, &state);
}

void HybridSpotMarketStrategy::reset() {
    initialize();
    initBackend(backends.ipoptBackend,
            backendConfigs.ipoptConfig,
            config.utilizationRatio.value());
    initBackend(backends.lrBackend,
            backendConfigs.lrConfig);
    backends.dcpBackend = nullptr;
    backends.bendersBackend = nullptr;
    lastUpdate=-1e100;
}

void HybridSpotMarketStrategy::setBackends(const sea::BackendHolder& holder) {
    if (holder.ipoptBackend != nullptr) {
        backends.ipoptBackend = holder.ipoptBackend;
        backends.ipoptBackend->setUtilizationRatio(config.utilizationRatio.value());
    }
    if (holder.lrBackend != nullptr) {
        backends.lrBackend = holder.lrBackend;
    }
    backends.dcpBackend = nullptr;
    backends.bendersBackend = nullptr;
}


} // namespace strategy
} // namespace sea
