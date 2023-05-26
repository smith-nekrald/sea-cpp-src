#include "abstract_allotment_strategy.h"

namespace sea {
namespace strategy {

AbstractAllotmentStrategy::AbstractAllotmentStrategy(
            const AllotmentStrategyConfig& aConfig,
            const BackendConfigHolder& aBackendConfigs,
            std::string aName)
        : backendConfigs(aBackendConfigs)
        , config(aConfig)
        , name(aName) {
    utilizationRatio = config.defaultUtilizationRatio.value();
    valueEstimation = 0;
    logCreatedAbstractAllotmentStrategy();
}

void AbstractAllotmentStrategy::hardReset() {
    logHardResetInAbstractAllotmentStrategy();
    backends.ipoptBackend = nullptr;
    backends.lrBackend = nullptr;
    backends.dcpBackend = nullptr;
    backends.bendersBackend = nullptr;

    decisionCopy.clear();
    valueEstimation = 0;
    reset();
}

} // namespace strategy
} // namespace sea
