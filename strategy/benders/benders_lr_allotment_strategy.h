#pragma once


#include "../abstract_allotment_strategy.h"
#include "../../backend/benders/benders_allotment_backend.h"
#include "../../backend/lagrangian_relaxation/lagrangian_relaxation_backend.h"
#include "../../backend/ipopt/ipopt_backend.h"


namespace sea {
namespace strategy {

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


class BendersLRAllotmentStrategy : public AbstractAllotmentStrategy {
public:
    BendersLRAllotmentStrategy(const BendersLRAllotmentStrategyConfig& aConfig);

    virtual void reset() override;
    virtual DecisionManagerPtr provideAllotments() override;
    virtual ~BendersLRAllotmentStrategy() {};
private:
    const StrategyBendersInnerConfig innerConfig;
};


} // namespace strategy
} // namespace sea

