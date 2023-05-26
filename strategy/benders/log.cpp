#include "benders_lr_allotment_strategy.h"
#include "../../algorithm/state.h"

namespace sea {
namespace strategy {

void BendersLRAllotmentStrategy::logIteration(
        std::size_t iterId, double lrObjective,
        double ipoptObjective, double blendedObjective) const {
    auto& logger = logging::getAllotmentStrategyLogger(config.type.value());
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Benders allotment strategy: iteration " << iterId << "\n";
    stream << "lrObjective = " << lrObjective << "\n";
    stream << "ipoptObjective = " << ipoptObjective << "\n";
    stream << "blendedObjective = " << blendedObjective << "\n";
}

void BendersLRAllotmentStrategy::logUpdateGroup(const VariableGroup& bestGroup) const {
    auto& logger = logging::getAllotmentStrategyLogger(config.type.value());
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Changing bestGroup into currentGroup" << "\n";
    stream << "Best blended objective now: " << bestGroup.blendedObjective << "\n";
    stream << "Best Ipopt objective now: " << bestGroup.ipoptObjective << "\n";
    stream << "Best LR objective now: " << bestGroup.lrObjective << "\n";
}

void BendersLRAllotmentStrategy::logStopIterations() const {
    auto& logger = logging::getAllotmentStrategyLogger(config.type.value());
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Stopping iterations of benders -- solution is found" << "\n";
}

} // namespace strategy
} // namespace sea

