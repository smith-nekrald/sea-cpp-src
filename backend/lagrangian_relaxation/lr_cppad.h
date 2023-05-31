/**
 * @file lr_cppad.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Functional approach to the method based on the Lagrangian Relaxation.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "lagrangian_relaxation_backend.h"
#include "structures.h"
#include "index.h"

#include <stdexcept>
#include <cppad/cppad.hpp>
#include <cppad/core/value.hpp>
#include <cppad/core/var2par.hpp>

namespace sea {
namespace backend {

/**
 * @brief Uniform method to extract double value. Implementation for double.
 * 
 * @param entry The entry to consider.
 * @return double The double value. In this case, equals to entry.
 */
double Value(const double& entry);
/**
 * @brief Uniform method to extract double value. Implementation for CppAD<double>.
 * 
 * @param entry The entry to consider.
 * @return double The double value, extracted (or computed) from entry.
 */
double Value(const CppAD::AD<double>& entry);
/**
 * @brief Uniform method to extract double value. Implementation for CppAD<CppAD<double>>.
 * 
 * @param entry The entry to consider.
 * @return double The double value, extracted or computed from entry.
 */
double Value(const CppAD::AD<CppAD::AD<double>>& entry);

/**
 * @brief Evaluate LR objective function in point.
 * 
 * @param point The point in dual space for evaluation.
 * @param state The trajectory evaluation state..
 * @param inputManager The Manager with InputData.
 * @param linksManager The Manager with InputLinks.
 * @param indexManager The Manager with LR Index.
 * @param decision The Manager with Decision.
 * @param ignoreSpot If true, spot market is ignored.
 * @param l2CoeffReg Coefficient of L2 regularization, when relevant.
 * @param mean Mean value, for centering L2 regularization and better approximation.
 * @return The value of LR objective function in point.
 */
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

/**
 * @brief Computes subgradient with CppAD.
 * 
 * @param point The DualVariables point.
 * @param state The state of trajectory evaluation.
 * @param inputManager The Manager with InputData.
 * @param linksManager The Manager with InputLinks.
 * @param indexManager The Manager with LR Index.
 * @param decisionManager The Manager with Decision.
 * @param ignoreSpot If true, spot market is ignored.
 * @param l2CoeffReg Coefficient of regularization.
 * @param mean Mean dual value, for centering.
 * @return Subgradient in point.
 */
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
