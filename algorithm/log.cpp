// Implements module-related logging.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "strategic_algorithm.h"

#include "../logging/logging.h"

namespace sea {
namespace algo {

void StrategicAlgorithm::logStrategicAlgorithmCreated() const {
    logging::getAlgorithmLogger().debug("Created strategic algorithm.");
    logging::getAlgorithmLogger().debug(getName());
}

void StrategicAlgorithm::logSelectedAllotments(ConstDecisionManagerPtr decisionManager) const {
    auto& logger = logging::getAlgorithmLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "StrategicAlgoritm: printing selected allotments.\n";
    auto& allotments = decisionManager->getConstData().allotmentAccepted;
    for (const auto& item : allotments) {
        stream << int(item) << " ";
    }
}

} // namespace algo
} // namespace sea
