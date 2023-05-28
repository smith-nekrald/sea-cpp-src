// Implements methods defined in lr_cppad.h. Essentially, subgradient estimation with
// CppAD and function evaluation in point. Also, all related API.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "lr_cppad.hpp"

namespace sea {
namespace backend {

using ArcType=InputData::Arc::Type;
using EventType=InputData::Event::Type;
using CppAD::AD;

double Value(const double& entry) {
    return entry;
}

double Value(const CppAD::AD<double>& entry) {
    return CppAD::Value(CppAD::Var2Par(entry));
}

double Value(const CppAD::AD<CppAD::AD<double>>& entry) {
    return Value(CppAD::Value(CppAD::Var2Par(entry)));
}

double evaluateFunctionInPoint(
            const DualVariables& point,
            const State& state,
            ConstInputManagerPtr inputManager,
            ConstLinksManagerPtr linksManager,
            ConstLRIndexManagerPtr indexManager,
            DecisionManagerPtr decisionManager,
            bool ignoreSpot,
            double l2RegConstant,
            DualVariables mean) {
    return computeFunctionValue<double>(
        inputManager->getConstData(), linksManager->getConstData(), state,
        indexManager->getConstData(), state.timeParameters, decisionManager->get(),
        point, ignoreSpot, l2RegConstant, mean);
}

SubgradientOptimizationParameters estimateSubgradientWithCppAD(
        const DualVariables& point,
        const State& state,
        ConstInputManagerPtr inputManager,
        ConstLinksManagerPtr linksManager,
        ConstLRIndexManagerPtr indexManager,
        DecisionManagerPtr decisionManager,
        bool ignoreSpot,
        double l2RegConstant,
        DualVariables mean) {
    unsigned dualCount = point.muVariables.size() + point.lambdaVariables.size();
    vector<double> domain(dualCount);
    vector<AD<double>> dualVector(dualCount);
    for (unsigned idx = 0; idx < point.lambdaVariables.size(); ++idx) {
        domain[idx] = point.lambdaVariables[idx];
        dualVector[idx] = point.lambdaVariables[idx];
    }
    for (unsigned ind = 0; ind < point.muVariables.size(); ++ind) {
        domain[ind + point.lambdaVariables.size()] = point.muVariables[ind];
        dualVector[ind + point.lambdaVariables.size()] = point.muVariables[ind];
    }
    CppAD::Independent(dualVector);
    DualTemplate<AD<double>> dualAD;
    dualAD.lambdaVariables = vector<AD<double>>(
            dualVector.begin(), dualVector.begin() + point.lambdaVariables.size());
    dualAD.muVariables = vector<AD<double>>(
            dualVector.begin() + point.lambdaVariables.size(), dualVector.end());

    const unsigned OBJ_SIZE = 1;
    vector<AD<double>> objective(OBJ_SIZE);
    objective[0] = computeFunctionValue(
            inputManager->getConstData(), linksManager->getConstData(), state,
            indexManager->getConstData(), state.timeParameters, decisionManager->get(),
            dualAD, ignoreSpot, l2RegConstant, mean);
    CppAD::ADFun<double> map(dualVector, objective);
    vector<double> subgradient(dualCount);
    subgradient = map.Jacobian(domain);
    SubgradientOptimizationParameters result;
    result.lambdaSubgradient = vector<double>(subgradient.begin(), subgradient.begin() +
            point.lambdaVariables.size());
    result.muSubgradient = vector<double>(subgradient.begin() +
            point.lambdaVariables.size(), subgradient.end());
    result.functionValue = Value(objective[0]);
    return result;
}

} // namespace backend
} // namespace sea

