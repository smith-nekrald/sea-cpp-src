// Loggint for ipopt module.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "ipopt_spot_strategy.h"
#include "ipopt_allotment_strategy.h"

namespace sea {
namespace strategy {

void IpoptSpotMarketStrategy::logEnteredUpdateParams(bool needUpdate) const {
    logging::getSpotStrategyLogger(sea::SpotStrategyType::IPOPT).debugStream()
            << "Entered IpoptSpot:: updateParams.";
    logging::getSpotStrategyLogger(sea::SpotStrategyType::IPOPT).debugStream()
            << "If update is required? " << needUpdate;
}

void IpoptSpotMarketStrategy::logLeftUpdateParams() const {
    logging::getSpotStrategyLogger(sea::SpotStrategyType::IPOPT).debugStream()
        << "Left updateParams.";
}

void IpoptAllotmentStrategy::logAllotmentSelection(const vector<bool>& selection) const {
    auto& logger = logging::getAllotmentStrategyLogger(config.type.value());
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "IpoptAllotmentStrategy:: print selection.\n";
    stream << "Selection size = " << selection.size() << "\n";
    for (const auto& value : selection) {
        stream << value << " ";
    }
}

void IpoptAllotmentStrategy::logUpdatedDecision(ConstDecisionManagerPtr result) const {
    auto& logger = logging::getAllotmentStrategyLogger(config.type.value());
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "IpoptAllotmentStrategy:: print answer. \n";
    stream << result->getConstData();
}

} // namespace strategy
} // namespace sea
