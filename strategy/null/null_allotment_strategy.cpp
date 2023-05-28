// Implements logic defined in null allotment strategy.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "null_allotment_strategy.h"
#include "../../logging/logging.h"

#include <limits>

namespace sea {
namespace strategy {

DecisionManagerPtr NullAllotmentStrategy::provideAllotments() {
    if (decisionCopy.find(utilizationRatio) != decisionCopy.end()) {
        return decisionCopy[utilizationRatio];
    }
    ManagerConfig decisionConfig = {
        backendConfigs.ipoptConfig.needMemory,
        "decision_" + makeUniqueFileName(),
        true
    };
    DecisionManagerPtr result = std::make_shared<DataManager<Decision>>(decisionConfig);
    createDecision(config.inputManager->getConstData(), result->get());
    auto& decision = result->getData();
    fillZero<bool>(decision.allotmentAccepted);
    fillZero<unsigned>(decision.hiredY);
    fillZero<unsigned>(decision.nonEmptyContainersQ);
    for (auto& subvector : decision.offHiredInPortS) {
        fillZero<unsigned>(subvector);
    }
    fillZero<unsigned>(decision.emptyContainersZ);
    const double INF = std::numeric_limits<double>::max();
    for (auto& subvector : decision.prices) {
        for (auto& entry : subvector) {
            entry.second = INF;
        }
    }
    for (auto& subvector : decision.allotmentContainersQ) {
        for (auto& entry : subvector) {
            entry.second = 0;
        }
    }
    if (config.storeInitialDecision) {
        decisionCopy[utilizationRatio] = result->deepCopy();
        decisionCopy[utilizationRatio]->release();
    }
    valueEstimation = 0;
    return result;
}

void NullAllotmentStrategy::reset() {
    backends.ipoptBackend = nullptr;
    backends.dcpBackend = nullptr;
    backends.bendersBackend = nullptr;
    backends.lrBackend = nullptr;
}

} // namespace strategy
} // namespace sea
