#pragma once

#include "../abstract_allotment_strategy.h"
#include "../../backend/benders/benders_allotment_backend.h"
#include "../../backend/lagrangian_relaxation/lagrangian_relaxation_backend.h"
#include "../../backend/ipopt/ipopt_backend.h"

#include <limits>

namespace sea {
namespace strategy {

const double DOUBLE_INF = std::numeric_limits<double>::max();

enum class InitializationType {
    IPOPT_ALLOTMENTS,
    DETERMINISTIC_CUTTING_PLANE_ALLOTMENTS,
    ZERO_ALLOTMENTS
};

struct StrategyBendersInnerConfig {
    double objectiveBlending = 1.0;
    ConvergenceCriterionType stopStrategy = ConvergenceCriterionType::BOTH;
    std::size_t iterationCount = 30;
    InitializationType initType = InitializationType::IPOPT_ALLOTMENTS;
    double bendersStopPrecision = 1e-5;
};

struct BendersLRAllotmentStrategyConfig {
    BackendConfigHolder backendConfigs;
    AllotmentStrategyConfig abstractConfig;
    StrategyBendersInnerConfig config;
};

struct VariableGroup {
    double blendedObjective = -DOUBLE_INF;
    double ipoptObjective = -DOUBLE_INF;
    double lrObjective = -DOUBLE_INF;

    vector<bool> allotmentSelection;
};

bool operator<(const VariableGroup& lhs, const VariableGroup& rhs);

double objectiveBlending(
        double deterministicObjective, double methodObjective, double weight = 1.0);

class BendersLRAllotmentStrategy : public AbstractAllotmentStrategy {
public:
    BendersLRAllotmentStrategy(const BendersLRAllotmentStrategyConfig& aConfig);

    virtual void reset() override;
    virtual DecisionManagerPtr provideAllotments() override;
    virtual ~BendersLRAllotmentStrategy() {};
private:
    void logIteration(std::size_t iterId, double lrObjective,
            double ipoptObjective, double blendedObjective) const;
    void logUpdateGroup(const VariableGroup& group) const;
    void logStopIterations() const;
private:
    const StrategyBendersInnerConfig innerConfig;
};

} // namespace strategy
} // namespace sea

