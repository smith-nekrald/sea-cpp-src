#include "abstract_allotment_strategy.h"

namespace sea {
namespace strategy  {

AbstractAllotmentStrategy::AbstractAllotmentStrategy(
            const AllotmentStrategyConfig& aConfig,
            const BackendConfigHolder& aBackendConfigs,
            std::string aName)
        : backendConfigs(aBackendConfigs)
        , config(aConfig)
        , name(aName) {
    logging::getAllotmentStrategyLogger(config.type.value()).info(
            "AbstractAllotmentStrategy created.");
    utilizationRatio = config.defaultUtilizationRatio.value();
    valueEstimation = 0;
}

void AbstractAllotmentStrategy::hardReset() {

    logging::getAllotmentStrategyLogger(config.type.value()).debug(
            "AbstractAllotmentStrategy::hardReset() is called.");

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
