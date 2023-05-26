#include "dcp_allotment_strategy.h"

namespace sea {
namespace strategy {

void DetCutPlaneAllotmentStrategy::logPrintAnswerInProvideAllotments(
        ConstDecisionManagerPtr decision) const {
    auto& logger = logging::getAllotmentStrategyLogger(config.type.value());
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "DetCutPlaneAllotmentStrategy:: print answer. \n";
    stream << decision->getConstData();
}

} // namespace strategy
} // namespace sea
