// Implements Validator-related logging.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "validator.h"
#include "../logging/logging.h"

namespace sea {

void Validator::logEvaluatingBendersInBendersCheck() const {
    auto& logger = logging::getValidationLogger();
    logger.debugStream() << "Validator::bendersCheck().";
    logger.debugStream() << "Evaluating Benders proposal: ";
}

void Validator::logEvaluatingIpoptInBendersCheck() const {
    auto& logger = logging::getValidationLogger();
    logger.debugStream() << "Validator::bendersCheck().";
    logger.debugStream() << "Evaluating Ipopt proposal: ";
}

void Validator::logScoresInProcessSelection(
        double ipoptScore, double lrScore, double ipoptFullProfit, double lrFullProfit) const {
    auto& logger = logging::getValidationLogger();
    if (lrScore * ipoptScore < 0) {
        logger.errorStream() << "Error detected: < 0.";
    }
    logger.infoStream() << "Predicted ipoptScore = " << ipoptScore;
    logger.infoStream() << "Predicted lrScore = " << lrScore;
    logger.infoStream() << "Evaluated ipopt_spot: " << ipoptFullProfit;
    logger.infoStream() << "Evaluated lr_spot: " << lrFullProfit;
}

} // namespace sea
