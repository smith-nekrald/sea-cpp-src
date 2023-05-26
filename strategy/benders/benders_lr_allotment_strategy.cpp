#include "benders_lr_allotment_strategy.h"
#include "../../algorithm/state.h"

#include <limits>

namespace sea {
namespace strategy {

const double INF = std::numeric_limits<double>::max();

double objectiveBlending(double deterministicObjective, double magicObjective, double weight = 1.0) {
    return weight * magicObjective + (1.0 - weight) * deterministicObjective;
}

struct VariableGroup {
    double blendedObjective = -INF;
    double ipoptObjective = -INF;
    double lrObjective = -INF;

    vector<bool> allotmentSelection;
};

bool operator<(const VariableGroup& lhs, const VariableGroup& rhs) {
    return lhs.blendedObjective < rhs.blendedObjective;
}

BendersLRAllotmentStrategy::BendersLRAllotmentStrategy(
        const BendersLRAllotmentStrategyConfig& aConfig)
        : AbstractAllotmentStrategy(
                aConfig.abstractConfig,
                aConfig.backendConfigs,
                "benders_allotment")
        , innerConfig(aConfig.config) {
    reset();
}

DecisionManagerPtr BendersLRAllotmentStrategy::provideAllotments() {
    if (decisionCopy.find(utilizationRatio) != decisionCopy.end()) { return decisionCopy[utilizationRatio]->deepCopy(); }
    State state;
    initState(config.inputManager->getConstData(), &state);
    vector<bool> allotments;
    if (innerConfig.initType == InitializationType::ZERO_ALLOTMENTS) {
        allotments.assign(config.inputManager->getConstData().allotments.size(), 0);
    } else if (innerConfig.initType == InitializationType::IPOPT_ALLOTMENTS) {
        allotments = backends.ipoptBackend->provideAllotments();
    } else if (innerConfig.initType == InitializationType::DETERMINISTIC_CUTTING_PLANE_ALLOTMENTS) {
        throw std::runtime_error("DETERMINISTIC_CUTTING_PLANE_ALLOTMENTS is not implemented yet");
    } else {
        throw std::logic_error("Unknown initType!");
    }
    bool useObjDiff = (innerConfig.stopStrategy == ConvergenceCriterionType::BOTH ||
                innerConfig.stopStrategy == ConvergenceCriterionType::OBJECTIVE_CHANGE);
    bool useVariableDiff = (innerConfig.stopStrategy == ConvergenceCriterionType::BOTH ||
                innerConfig.stopStrategy == ConvergenceCriterionType::VARIABLE_CHANGE);
    VariableGroup lastGroup;
    lastGroup.allotmentSelection = allotments;
    DualVariables ipoptDuals;
    auto decisionManager = backends.ipoptBackend->bendersQuery(
            lastGroup.allotmentSelection,
            &lastGroup.ipoptObjective,
            &ipoptDuals);
    backends.lrBackend->provideIpoptDuals(ipoptDuals);
    auto duals = backends.lrBackend->provideDuals(state, decisionManager, nullptr, &lastGroup.lrObjective);
    VariableGroup currentGroup, bestGroup = lastGroup;

    for (std::size_t iterId = 0; iterId < innerConfig.iterationCount; ++iterId) {

        backends.bendersBackend->addDuals(duals);
        currentGroup.allotmentSelection = backends.bendersBackend->makeAllotments(decisionManager);
        decisionManager = backends.ipoptBackend->bendersQuery(currentGroup.allotmentSelection, &currentGroup.ipoptObjective, &ipoptDuals);
        backends.lrBackend->provideIpoptDuals(ipoptDuals);
        duals = backends.lrBackend->provideDuals(
                state, decisionManager, nullptr, &currentGroup.lrObjective);

        currentGroup.blendedObjective = objectiveBlending(currentGroup.ipoptObjective, currentGroup.lrObjective, innerConfig.objectiveBlending);
        {
            auto& logger = logging::getAllotmentStrategyLogger(config.type.value());
            auto stream = logger.getStream(log4cpp::Priority::DEBUG);
            stream << "Benders allotment strategy: iteration " << iterId << "\n";
            stream << "lrObjective = " << currentGroup.lrObjective << "\n";
            stream << "ipoptObjective = " << currentGroup.ipoptObjective << "\n";
            stream << "blendedObjective = " << currentGroup.blendedObjective << "\n";
        }
        if (bestGroup < currentGroup) {
            bestGroup = currentGroup;
            {
                auto& logger = logging::getAllotmentStrategyLogger(config.type.value());
                auto stream = logger.getStream(log4cpp::Priority::DEBUG);
                stream << "Changing bestGroup into currentGroup" << "\n";
                stream << "Best blended objective now: " << bestGroup.blendedObjective << "\n";
                stream << "Best ipopt objective now: " << bestGroup.ipoptObjective << "\n";
                stream << "Best lr objective now: " << bestGroup.lrObjective << "\n";
            }
        }
        bool stopObjective = !useObjDiff, stopVariables = !useVariableDiff;
        if (useObjDiff &&
                fabs (lastGroup.blendedObjective - currentGroup.blendedObjective)
                < innerConfig.bendersStopPrecision) {
            stopObjective = true;
        }
        if (useVariableDiff) {
            stopVariables = true;
            for (unsigned allotmentId = 0;
                    allotmentId < lastGroup.allotmentSelection.size(); ++allotmentId) {
                if (lastGroup.allotmentSelection[allotmentId] !=
                        currentGroup.allotmentSelection[allotmentId]) {
                    stopVariables = false;
                    break;
                }
            }
        }
        if (stopObjective && stopVariables) {
            {
                auto& logger = logging::getAllotmentStrategyLogger(config.type.value());
                auto stream = logger.getStream(log4cpp::Priority::DEBUG);
                stream << "Stopping iterations of benders -- solution is found" << "\n";
            }
            break;
        }
        lastGroup = currentGroup;
    }
    decisionManager = backends.ipoptBackend->bendersQuery(bestGroup.allotmentSelection);

    if (config.storeInitialDecision) {
        decisionCopy[utilizationRatio] = decisionManager->deepCopy();
        decisionCopy[utilizationRatio]->release();
    }
    valueEstimation = bestGroup.blendedObjective;
    return decisionManager;

}

void BendersLRAllotmentStrategy::reset() {
    initBackend(
        backends.ipoptBackend,
        backendConfigs.ipoptConfig,
        utilizationRatio);
    initBackend(
        backends.lrBackend,
        backendConfigs.lrConfig);
    initBackend(
        backends.bendersBackend,
        backendConfigs.bendersConfig);
    backends.dcpBackend = nullptr;
}

} //namespace strategy
} // namespace sea
