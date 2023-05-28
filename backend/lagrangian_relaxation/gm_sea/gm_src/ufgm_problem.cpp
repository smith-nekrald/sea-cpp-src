// Implements logic for BregmanOptimization Problem and ArgminOptimization Problem.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "ufgm_problem.h"
#include "gm_interfaces.h"

#include <cppad/ipopt/solve.hpp>

namespace gm {

using std::size_t;
using std::vector;

BregmanOptimizationProblem::BregmanOptimizationProblem(
            const std::vector<double>& grad_f_xk,
            const double ak,
            const std::vector<double>& pivot,
            std::shared_ptr<const IQDescriptor> qDescriptor,
            std::shared_ptr<const DoubleFunction> regularizer,
            std::shared_ptr<const DoubleFunction> prox,
            std::shared_ptr<const CppADFunction> adRegularizer,
            std::shared_ptr<const CppADFunction> adProx)
        : grad_f_xk(grad_f_xk)
        , ak(ak)
        , pivot(pivot)
        , qDescriptor(qDescriptor)
        , regularizer(regularizer)
        , prox(prox)
        , adRegularizer(adRegularizer)
        , adProx(adProx) {
}


void BregmanOptimizationProblem::operator()(ADvector& functions, const ADvector& variables) {
    auto constraints = qDescriptor->makeConstraints(variables);
    functions.resize(constraints.size() + 1);

    for (size_t idx = 0; idx < constraints.size(); ++idx) {
        functions[idx + 1] = constraints[idx];
    }
    auto regAD = adRegularizer->getValue(variables);
    assert(variables.size() == grad_f_xk.size());
    assert(variables.size() == pivot.size());
    BregmanDistance<double, CppAD::AD<double>> distance(prox, adProx);
    auto xiDistance = distance.getDistance(pivot, variables);

    CppAD::AD<double> scalarProduct = 0;
    for (size_t idx = 0; idx < variables.size(); ++idx) {
        scalarProduct += variables[idx] * grad_f_xk[idx];
    }
    auto objective = xiDistance + ak * (scalarProduct + regAD);
    functions[0] = objective;
}


ArgminOptimizationProblem::ArgminOptimizationProblem(
            const std::vector<double>& coeffAns,
            const std::vector<double>& x0,
            const double coeffReg,
            const double coeffFree,
            std::shared_ptr<const IQDescriptor> qDescriptor,
            std::shared_ptr<const DoubleFunction> regularizer,
            std::shared_ptr<const DoubleFunction> prox,
            std::shared_ptr<const CppADFunction> adRegularizer,
            std::shared_ptr<const CppADFunction> adProx)
    : coeffAns(coeffAns)
    , x0(x0)
    , coeffReg(coeffReg)
    , coeffFree(coeffFree)
    , qDescriptor(qDescriptor)
    , regularizer(regularizer)
    , prox(prox)
    , adRegularizer(adRegularizer)
    , adProx(adProx) {
}


void ArgminOptimizationProblem::operator()(ADvector& functions, const ADvector& variables) {
    auto constraints = qDescriptor->makeConstraints(variables);
    functions.resize(constraints.size() + 1);
    for (size_t idx = 0; idx < constraints.size(); ++idx) {
        functions[idx + 1] = constraints[idx];
    }
    auto regAD = adRegularizer->getValue(variables);

    BregmanDistance<double, CppAD::AD<double>> distance(prox, adProx);
    assert(variables.size() == x0.size());
    auto xiDistance = distance.getDistance(x0, variables);

    CppAD::AD<double> scalarProduct = 0;
    assert(variables.size() == coeffAns.size());
    for (size_t idx = 0; idx < variables.size(); ++idx) {
        scalarProduct += variables[idx] * coeffAns[idx];
    }
    auto objective = xiDistance + scalarProduct + regAD * coeffReg + coeffFree;
    functions[0] = objective;
}

} // namespace gm
