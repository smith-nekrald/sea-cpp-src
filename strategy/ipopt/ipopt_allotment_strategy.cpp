#include "ipopt_allotment_strategy.h"
#include "../../logging/logging.h"

namespace sea {
namespace strategy {

DecisionManagerPtr IpoptAllotmentStrategy::provideAllotments() {
    if (decisionCopy.find(utilizationRatio) != decisionCopy.end()) {
        return decisionCopy[utilizationRatio]->deepCopy();
    }
    vector<bool> selection = memorizedSelection;
    DecisionManagerPtr result;
    if (!selection.size()) {
        selection = backends.ipoptBackend->provideAllotments(&valueEstimation);
    }
    {
        auto& logger =
            logging::getAllotmentStrategyLogger(config.type.value());
        auto stream = logger.getStream(log4cpp::Priority::DEBUG);
        stream << "IpoptAllotmentStrategy:: print selection.\n";
        stream << "Selection size = " << selection.size() << "\n";
        for (const auto& value : selection) {
            stream << value << " ";
        }
    }
    result = backends.ipoptBackend->bendersQuery(
            selection, &valueEstimation);
    if (config.storeInitialDecision) {
        decisionCopy[utilizationRatio] = result->deepCopy();
        decisionCopy[utilizationRatio]->release();
    }
    {
        auto& logger =
            logging::getAllotmentStrategyLogger(config.type.value());
        auto stream = logger.getStream(log4cpp::Priority::DEBUG);
        stream << "IpoptAllotmentStrategy:: print answer. \n";
        stream << result->getConstData();
    }
    return result;
}

void IpoptAllotmentStrategy::reset() {
    initBackend(backends.ipoptBackend,
            backendConfigs.ipoptConfig,
            utilizationRatio);
    backends.dcpBackend = nullptr;
    backends.bendersBackend = nullptr;
    backends.lrBackend = nullptr;
}

void IpoptAllotmentStrategy::hardReset() {
    memorizedSelection = forcedAllotments;
    AbstractAllotmentStrategy::hardReset();
}

} // namespace strategy
} // namespace sea
