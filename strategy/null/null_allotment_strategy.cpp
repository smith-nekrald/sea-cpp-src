#include "null_allotment_strategy.h"
#include "../../logging/logging.h"

namespace sea {
namespace strategy {

template<typename Type>
void fillZero(std::vector<Type>& data) {
    for (std::size_t ind = 0; ind < data.size(); ++ind) {
        data[ind] = 0;
    }
}

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
    fillZero<ui32>(decision.hiredY);
    fillZero<ui32>(decision.nonEmptyContainersQ);
    for (auto& subvector : decision.offHiredInPortS) {
        fillZero<ui32>(subvector);
    }
    fillZero<ui32>(decision.emptyContainersZ);

    for (auto& subvector : decision.prices) {
        for (auto& entry : subvector) {
            entry.second = 1e100;
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
