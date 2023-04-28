#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>
#include <cppad/ipopt/solve.hpp>

namespace gm {

class UFGMOptimizer : public IGradientOptimizer {

public:
    UFGMOptimizer(
            std::shared_ptr<DoubleFunction> target,
            std::shared_ptr<DoubleFunction> regularizer,
            std::shared_ptr<DoubleFunction> prox,
            std::shared_ptr<CppADFunction> adTarget,
            std::shared_ptr<CppADFunction> adRegularizer,
            std::shared_ptr<CppADFunction> adProx,
            std::shared_ptr<IQDescriptor> qDescriptor,
            size_t maxIter,
            double eps,
            double L0,
            bool keepStory);
    virtual std::vector<double> optimize(const std::vector<double>& initial) const;

protected:
    virtual std::vector<double> compute_argmin_bregman(
            const std::vector<double>& grad_f_xk,
            double ak,
            const std::vector<double>& bregmanPivot,
            const std::vector<double>& init) const;
    virtual std::vector<double> compute_argmin_psi(
            const std::vector<double>& coeffAns,
            double coeffReg,
            double coeffFree,
            const std::vector<double>& x0,
            const std::vector<double>& init) const;

private:
    void setCppADToValues(const std::vector<double>& values,
            std::vector<CppAD::AD<double>>* variables) const;
    std::string makeIpoptOptions() const;

private:

    std::shared_ptr<const DoubleFunction>
        target,
        regularizer,
        prox;
    std::shared_ptr<const CppADFunction>
        adTarget,
        adRegularizer,
        adProx;

    std::shared_ptr<const IQDescriptor> qDescriptor;

    const size_t maxIter;
    const double
        eps,
        L0;
};

} // namespace gm

