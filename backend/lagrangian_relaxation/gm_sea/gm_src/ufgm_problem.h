#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>
#include <numeric>
#include <cppad/ipopt/solve.hpp>

namespace gm {

class BregmanOptimizationProblem {
public:
    typedef std::vector<CppAD::AD<double>> ADvector;

public:
    BregmanOptimizationProblem(
            const std::vector<double>& grad_f_xk,
            const double ak,
            const std::vector<double>& pivot,
            std::shared_ptr<const IQDescriptor> qDescriptor,
            std::shared_ptr<const DoubleFunction> regularizer,
            std::shared_ptr<const DoubleFunction> prox,
            std::shared_ptr<const CppADFunction> adRegularizer,
            std::shared_ptr<const CppADFunction> adProx);

    void operator()(
            ADvector& functions,
            const ADvector& variables);

private:
    const std::vector<double> grad_f_xk;
    const double ak;
    const std::vector<double> pivot;
    std::shared_ptr<const IQDescriptor> qDescriptor;
    std::shared_ptr<const DoubleFunction> regularizer, prox;
    std::shared_ptr<const CppADFunction> adRegularizer, adProx;
};


class ArgminOptimizationProblem {
public:
    typedef std::vector<CppAD::AD<double>> ADvector;

public:
    ArgminOptimizationProblem(
            const std::vector<double>& coeffAns,
            const std::vector<double>& x0,
            const double coeffReg,
            const double coeffFree,
            std::shared_ptr<const IQDescriptor> qDescriptor,
            std::shared_ptr<const DoubleFunction> regularizer,
            std::shared_ptr<const DoubleFunction> prox,
            std::shared_ptr<const CppADFunction> adRegularizer,
            std::shared_ptr<const CppADFunction> adProx);

    void operator()(
            ADvector& functions,
            const ADvector& variables);

private:
    const std::vector<double> coeffAns;
    const std::vector<double> x0;
    const double coeffReg;
    const double coeffFree;
    std::shared_ptr<const IQDescriptor> qDescriptor;
    std::shared_ptr<const DoubleFunction> regularizer, prox;
    std::shared_ptr<const CppADFunction> adRegularizer, adProx;
};


} // namespace gm
