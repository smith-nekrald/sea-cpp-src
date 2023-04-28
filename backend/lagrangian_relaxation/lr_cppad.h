#pragma once

#include "lagrangian_relaxation_backend.h"
#include "structures.h"
#include "index.h"

#include <stdexcept>

namespace sea {
namespace backend {

double evaluateFunctionInPoint(
        const DualVariables& point,
        const State& state,
        ConstInputManagerPtr inputManager,
        ConstLinksManagerPtr linksManager,
        ConstLRIndexManagerPtr indexManager,
        DecisionManagerPtr decision,
        bool ignoreSpot=false,
        double l2CoeffReg=0,
        DualVariables mean=DualVariables());

SubgradientOptimizationParameters estimateSubgradientWithCppAD(
        const DualVariables& point,
        const State& state,
        ConstInputManagerPtr inputManager,
        ConstLinksManagerPtr linksManager,
        ConstLRIndexManagerPtr indexManager,
        DecisionManagerPtr decisionManager,
        bool ignoreSpot=false,
        double l2CoeffReg=0,
        DualVariables mean=DualVariables());


} // namespace backend
} // namespace sea
