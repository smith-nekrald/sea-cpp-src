// Implements logic for dcp allotment strategy module.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
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
