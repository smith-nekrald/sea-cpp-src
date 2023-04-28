#include "strategic_algorithm.h"

#include "../logging/logging.h"

namespace sea {

namespace algo {


StrategicAlgorithm:: StrategicAlgorithm(const StrategicAlgorithmConfig& aConfig)
    : config(aConfig) {
        logging::getAlgorithmLogger().debug("Created strategic algorithm.");
        logging::getAlgorithmLogger().debug(getName());
}

ConstDecisionManagerPtr StrategicAlgorithm::makeDecision() {
    auto& logger = logging::getAlgorithmLogger();
    ConstDecisionManagerPtr decisionManagerResponse = nullptr;
    if (!allotmentsAsked) {
        auto decisionManager = config.allotmentStrategy->provideAllotments();
        assert(decisionManager != nullptr);
        config.spotMarketStrategy->supplyAllotmentDecision(
                decisionManager);
        config.allotmentStrategy->reset();
        allotmentsAsked = true;
        decisionManagerResponse = decisionManager;
        if (config.spotMarketStrategy->needWarmBackends()) {
            config.spotMarketStrategy->setBackends(
                    config.allotmentStrategy->getBackends());
        }
    } else {
        decisionManagerResponse
            = config.spotMarketStrategy->makeDecision();
        {
            auto stream = logger.getStream(log4cpp::Priority::DEBUG);
            stream << "StrategicAlgoritm: printing selected allotments.\n";
            auto& allotments =
                decisionManagerResponse->getConstData().allotmentAccepted;
            for (const auto& item : allotments) {
               stream << int(item) << " ";
            }
        }
    }
    return decisionManagerResponse;
}

void StrategicAlgorithm::submitAction(ConstActionManagerPtr actionManager) {
    assert(allotmentsAsked);
    config.spotMarketStrategy->submitAction(actionManager);
}

void StrategicAlgorithm::reset() {
    config.allotmentStrategy->reset();
    config.spotMarketStrategy->reset();
    allotmentsAsked = false;
}

void StrategicAlgorithm::synchronizeStrategies() {
    double spotUtilizationRatio = config.spotMarketStrategy->getUtilizationRatio();
    double allotmentUtilizationRatio = config.allotmentStrategy->getUtilizationRatio();
    double EPS = 1e-4;
    if (std::fabs(spotUtilizationRatio - allotmentUtilizationRatio) > EPS) {
        config.allotmentStrategy->setUtilizationRatio(spotUtilizationRatio);
    }
}

} // namespace algo
} // namespace sea
