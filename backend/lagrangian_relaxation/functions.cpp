#include "functions.h"
#include "../../logging/logging.h"

namespace sea {
namespace backend {

double vectorAbsDiffSum(const std::vector<double>& lhs, const std::vector<double>& rhs) {
    assert(lhs.size() == rhs.size());
    double result = 0;
    for (ui32 index = 0; index < lhs.size(); ++index) {
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

ui32 countNonZeroElements(const SubgradientOptimizationParameters& parameters, const double EPS) {
    ui32 result = 0;
    for (ui32 idLambda = 0; idLambda < parameters.lambdaSubgradient.size(); ++idLambda) {
        if (fabs(parameters.lambdaSubgradient[idLambda]) > EPS) {
            ++result;
        }
    }
    for (ui32 idMu = 0; idMu < parameters.muSubgradient.size(); ++idMu) {
        if (fabs(parameters.muSubgradient[idMu]) > EPS) {
            ++result;
        }
    }
    return result;
}

void printDualsToBackendLog(const DualVariables& duals, BackendType type) {
    auto& logger = logging::getBackendSubLogger(type);
    std::string muOutput = "MuVariables : ";
    for (const auto& mu: duals.muVariables) {
        muOutput += std::to_string(mu);
        muOutput += " ";
    }
    logger.debug(muOutput);
    std::string lambdaOutput = "LambdaVariables : ";
    for (const auto& lambda : duals.lambdaVariables) {
        lambdaOutput += std::to_string(lambda);
        lambdaOutput += " ";
    }
    logger.debug(lambdaOutput);
}



} // namespace backend
} // namespace sea
