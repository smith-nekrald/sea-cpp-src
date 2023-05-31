/**
 * @file structures.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements commonly used structures for Lagrangian Relaxation (and also Benders 
 * Decomposition, which is based on Lagrangian Relaxation).
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../dual_variables.h"

#include <vector>

namespace sea {
namespace backend {

/**
 * @brief Coefficients for allotment parameters for Benders Decomposition heuristic.
 * This is required for recalculation coefficients when moving between deterministic solutions.
 */
struct UCoefficients {
    /// @brief Coefficient per allotment. The size = # of allotments.
    vector<double> coefficients;
    /// @brief Free member in Benders linear expression. Equals to
    /// sum{lambda_ab * W_b} + theta(lambda, mu).
    double freeMember = 0; 
};

/**
 * @brief Parameters for subgradient optimization. Essentially, structure with subgradient
 * over lambda-variables and mu-variables, alongside function value.
 */
struct SubgradientOptimizationParameters {
    /// @brief Lambda subgradient in point. The size = # of related arcs.
    std::vector<double> lambdaSubgradient;  
    /// @brief Mu subgradient in point. The size = # of itineraries.
    std::vector<double> muSubgradient; 
    /// @brief Stores function value in point.
    double functionValue = 0;
};

/// @brief Simple naming for a pair with UCoefficients and Dual Variables.
using CoefficientUInfo = std::pair<UCoefficients, DualVariables>;

} // namespace backend
} // namespace sea
