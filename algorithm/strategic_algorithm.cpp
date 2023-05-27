// Implements methods declared in strategic_algorithm.h

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
#include "strategic_algorithm.h"

namespace sea {

namespace algo {


StrategicAlgorithm:: StrategicAlgorithm(
        const StrategicAlgorithmConfig& aConfig) : config(aConfig) {
    logStrategicAlgorithmCreated();
}

ConstDecisionManagerPtr StrategicAlgorithm::makeDecision() {
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
        decisionManagerResponse = config.spotMarketStrategy->makeDecision();
        logSelectedAllotments(decisionManagerResponse);
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
