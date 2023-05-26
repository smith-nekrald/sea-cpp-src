#include "dcp_allotment_strategy.h"
#include "../../logging/logging.h"

namespace sea {
namespace strategy {

DecisionManagerPtr DetCutPlaneAllotmentStrategy::provideAllotments() {
    if (decisionCopy.find(utilizationRatio) != decisionCopy.end()) {
        return decisionCopy[utilizationRatio]->deepCopy();
    }
    auto decision = backends.dcpBackend->provideAllotments(&valueEstimation);
    logPrintAnswerInProvideAllotments(decision);

    if (config.storeInitialDecision) {
        decisionCopy[utilizationRatio] = decision->deepCopy();
        decisionCopy[utilizationRatio]->release();
    }

    return decision;
}

void DetCutPlaneAllotmentStrategy::reset() {
    initBackend(
        backends.dcpBackend, backendConfigs.dcpConfig, utilizationRatio);
    backends.ipoptBackend = nullptr;
    backends.bendersBackend = nullptr;
    backends.lrBackend = nullptr;
}


} // namespace strategy
} // namespace sea
