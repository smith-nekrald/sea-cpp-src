// Logging for lr strategy module.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "lr_spot_market_strategy.h"

namespace sea {
namespace strategy {

void LRBasedSpotMarketStrategy::logDualsInUpdateParams(
        const InputData::Event& event) const {
    logging::getOutTestLogger().debugStream() << "Updated_Duals at realTime = "
        << event.realTime << " relativeTime =  "  << event.relativeTime;
    auto dualWriter = logging::getOutTestLogger().getStream(log4cpp::Priority::DEBUG);
    dualWriter << "\n";
    dualWriter << "LambdaVariables: ";
    for (const auto& lambdaV : duals.lambdaVariables) {
        dualWriter << lambdaV << " ";
    }
    dualWriter << "\n";
    dualWriter << "MuVariables: ";
    for (const auto& muV : duals.muVariables) {
        dualWriter << muV << " ";
    }
    dualWriter << "\n";
}

} // namespace strategy
} // namespace sea
