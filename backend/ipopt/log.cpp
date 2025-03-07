// Logging methods.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
// (c) Smith School of Business, 2025

#include "ipopt_backend.h"
#include "../lagrangian_relaxation/index.h"
#include "../../logging/logging.h"

#include <string>

namespace sea {
namespace backend {

using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;
using std::size_t;

// Logging Functions

void logOptions(const std::string& options) {
   logging::getBackendSubLogger(sea::BackendType::IPOPT).debug("Ipopt Options: " + options);
}

void logConstraints(
        const vector<double>& vlower,
        const vector<double>& vupper,
        const vector<double>& glower,
        const vector<double>& gupper,
        const vector<double>& variables,
        const IpoptIndexMap& index,
        BackendType backendType) {
    assert(index.variableCount == vlower.size() &&
            index.variableCount == vupper.size() &&
            index.variableCount == variables.size());
    assert(index.constraintCount == glower.size() &&
        index.constraintCount == gupper.size());
    auto& logger = logging::getBackendSubLogger(backendType);
    logger.debug("Variable constraints: ");
    for (size_t idx = 0; idx < index.variableCount; ++idx) {
        logger.debug(std::string("Variable ") + std::to_string(idx) + ": ");
        if (index.variableToDescription.find(idx) !=
                index.variableToDescription.end()) {
            logger.debug(index.variableToDescription.at(idx));
        } else {
            logger.debug("No description.");
        }
        logger.debug(std::to_string(vlower[idx]) + " = lower");
        logger.debug(std::to_string(vupper[idx]) + " = upper");
        logger.debug(std::to_string(variables[idx]) + " = value");
        if (vlower[idx] > vupper[idx]) {
            logger.error("Lower is greater than upper for variable IDX " + std::to_string(idx));
        }
    }
    logger.debug("Equation constraints: ");
    for (size_t idx = 0; idx < index.constraintCount; ++idx) {
        logger.debug(std::string("Constraint ") + std::to_string(idx) + ": ");
        if (index.constraintToDescription.find(idx) != index.constraintToDescription.end()) {
            logger.debug(index.constraintToDescription.at(idx));
        } else {
            logger.debug("No description.");
        }
        logger.debug(std::to_string(glower[idx]) + " = lower");
        logger.debug(std::to_string(gupper[idx]) + " = upper");
        if (glower[idx] > gupper[idx]) {
            logger.error("Lower is greater than upper for constraint IDX " + std::to_string(idx));
        }

    }
}

void logSelectedAllotments(const vector<bool>& acceptedAllotments, BackendType backendType) {
    auto& logger = logging::getBackendSubLogger(backendType);
    auto stream = logger.getStream(log4cpp::Priority::INFO);
    stream << "These are the accepted allotments: " << "\n";
    for (auto value : acceptedAllotments) {
        stream << value << " ";
    }
}

void IpoptBackend::logIndexMapStats() const {
    auto ipoptLogger = logging::getBackendSubLogger(BackendType::IPOPT)
        << log4cpp::Priority::DEBUG;
    printIndexMapStats(ipoptLogger, indexManager->getConstData());
}

} // namespace backend
} //namespace sea
