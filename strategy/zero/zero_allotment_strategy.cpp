// Implements Zero Allotment Strategy logic.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "zero_allotment_strategy.h"
#include "../../logging/logging.h"

namespace sea {
namespace strategy {

DecisionManagerPtr ZeroAllotmentStrategy::provideAllotments() {
    if (decisionCopy.find(utilizationRatio) != decisionCopy.end()) {
        return decisionCopy[utilizationRatio]->deepCopy();
    }
    vector<bool> selection(config.inputManager->getConstData().allotments.size(), false);
    auto result = backends.ipoptBackend->bendersQuery(selection, &valueEstimation);
    if (config.storeInitialDecision) {
        decisionCopy[utilizationRatio] = result->deepCopy();
        decisionCopy[utilizationRatio]->release();
    }
    return result;
}

void ZeroAllotmentStrategy::reset() {
    initBackend(backends.ipoptBackend, backendConfigs.ipoptConfig);
    backends.dcpBackend = nullptr;
    backends.bendersBackend = nullptr;
    backends.lrBackend = nullptr;
}

void ZeroAllotmentStrategy::hardReset() {
    AbstractAllotmentStrategy::hardReset();
}

} // namespace strategy
} // namespace sea
