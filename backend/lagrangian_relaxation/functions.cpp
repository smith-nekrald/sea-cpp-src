// Commonly used functions.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "functions.h"

namespace sea {
namespace backend {

double vectorAbsDiffSum(const std::vector<double>& lhs, const std::vector<double>& rhs) {
    assert(lhs.size() == rhs.size());
    double result = 0;
    for (unsigned index = 0; index < lhs.size(); ++index) {
        result += fabsl(lhs[index] - rhs[index]);
    }
    return result;
}

double dualAbsDiffSum(const DualVariables& lhs, const DualVariables& rhs) {
    return vectorAbsDiffSum(lhs.lambdaVariables, rhs.lambdaVariables) +
        vectorAbsDiffSum(lhs.muVariables, rhs.muVariables);
}

double dualL2Norm(const DualVariables& duals) {
    double norm = 0;
    for (const auto& item : duals.lambdaVariables) {
        norm += item * item;
    }
    for (const auto& item : duals.muVariables) {
        norm += item * item;
    }
    return sqrt(norm);
}

double dualL1Norm(const DualVariables& duals) {
    double norm = 0;
    for (const auto& item : duals.lambdaVariables) {
        norm += fabsl(item);
    }
    for (const auto& item : duals.muVariables) {
        norm += fabsl(item);
    }
    return norm;
}

unsigned countNonZeroElements(
        const SubgradientOptimizationParameters& parameters, const double EPS) {
    unsigned result = 0;
    for (unsigned idLambda = 0; idLambda < parameters.lambdaSubgradient.size(); ++idLambda) {
        if (fabs(parameters.lambdaSubgradient[idLambda]) > EPS) {
            ++result;
        }
    }
    for (unsigned idMu = 0; idMu < parameters.muSubgradient.size(); ++idMu) {
        if (fabs(parameters.muSubgradient[idMu]) > EPS) {
            ++result;
        }
    }
    return result;
}


} // namespace backend
} // namespace sea
