#pragma once

#include "../dual_variables.h"

#include <vector>

namespace sea {
namespace backend {

struct UCoefficients {
    vector<double> coefficients; // size = # of allotments
    double freeMember = 0; // sum{lambda_ab * W_b} + theta(lambda, mu)
    // This is required for recalculation coefficients when
    // moving between deterministic solutions.
};

struct SubgradientOptimizationParameters {
    std::vector<double> lambdaSubgradient; // size = # of related arcs
    std::vector<double> muSubgradient; // size = # of itineraries
    double functionValue = 0;
};

using CoefficientUInfo = std::pair<UCoefficients, DualVariables>;

} // namespace backend
} // namespace sea
