/**
 * @file ufgm_optimizer.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Universal Fast Gradient Optimization Method. See description at Yuri Nesterov
 * HSE lecture 2 on slide 25.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>
#include <cppad/ipopt/solve.hpp>

namespace gm {

/**
 * @brief Implements Universal Fast Gradient Method optimization, defined at the Yuri
 * Nesterov HSE lecture 2 on slide 25.
 */
class UFGMOptimizer : public IGradientOptimizer {

public:
    /**
     * @brief UFGM Optimizer constructor.
     *
     * @param target Object to compute function value and subgradient in point.
     * @param regularizer Object to compute regularizer value and subgradient in point.
     * @param prox Object to compute prox-function value and subgradient in point.
     * @param adTarget Object to compute function value and subgradient in CppAD form.
     * @param adRegularizer Object to compute regularizer value and subgradient in CppAD form.
     * @param adProx Object to compute prox-function value and subgradient in CppAD form.
     * @param qDescriptor Object to compute constraints for projected optimization.
     * @param maxIter Maximal number of iterations.
     * @param eps Desired precision.
     * @param L0 value from the UFGM description. Any L0 > 0 fits.
     * @param keepStory If true, keeps story of evaluations.
     */
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
    /**
     * @brief Start iterative optimization.
     *
     * @param initial Initial point to start from.
     * @return The optimal solution (if converged).
     */
    virtual std::vector<double> optimize(const std::vector<double>& initial) const;

protected:
    /**
     * @brief Solves optimization problem at Step 2 of the UFGM formulation
     * on HSE slide 25 at lecture 2.
     *
     * @param grad_f_xk The gradient in point xk.
     * @param ak The coefficient ak.
     * @param bregmanPivot The bregman pivot, corresponds to vk on slide.
     * @param init Point to initialize optimization.
     * @return The solution to the Bregman Optimization argmin problem (formulated on
     * HSE slide 25 at lecture 2).
     */
    virtual std::vector<double> computeArgMinBregman(
            const std::vector<double>& grad_f_xk,
            double ak,
            const std::vector<double>& bregmanPivot,
            const std::vector<double>& init) const;
    /**
     * @brief Solves optimization problem at step 1 of the UFGM formulation
     * on HSE slide 25 at lecture 2.
     *
     * @param coeffAns The linear variable coefficient in the objective.
     * @param coeffReg The regularizer coefficient in the objective.
     * @param coeffFree The free coefficient in the objective.
     * @param x0 Starting reference point x0.
     * @param init Initialization for variables in Ipopt optimization.
     * @return The solution to the Argmin Optimization problem (step 1 at HSE slide 25, lecture 2).
     */
    virtual std::vector<double> computeArgMinPhi(
            const std::vector<double>& coeffAns,
            double coeffReg,
            double coeffFree,
            const std::vector<double>& x0,
            const std::vector<double>& init) const;

private:
    /**
     * @brief Sets CppAD variables point values to the specific initialization point.
     * Allows to accelerate optimization.
     *
     * @param values The values to set for the variable initialization.
     * @param variables Variables to provide with initialization.
     */
    void setCppADToValues(const std::vector<double>& values,
            std::vector<CppAD::AD<double>>* variables) const;
    /**
     * @brief Formulates Ipopt optimization options.
     *
     * @return String with Ipopt optimization options.
     */
    std::string makeIpoptOptions() const;

private:
    /// @brief Object to compute target function value and subgradient in point.
    std::shared_ptr<const DoubleFunction> target;
    /// @brief Object to compute regularizer value and subgradient in point.
    std::shared_ptr<const DoubleFunction> regularizer;
    /// @brief Object to compute prox-function value and subgradient in point.
    std::shared_ptr<const DoubleFunction> prox;

    /// @brief Object to compute target function value and subgradient in CppAD variable form.
    std::shared_ptr<const CppADFunction> adTarget;
    /// @brief Object to compute regularizer value and subgradient in CppAD variable form.
    std::shared_ptr<const CppADFunction> adRegularizer;
    /// @brief Object to compute prox-function value and subgradient in CppAD variable form.
    std::shared_ptr<const CppADFunction> adProx;

    /// @brief Object to compute constraints for projected optimization.
    std::shared_ptr<const IQDescriptor> qDescriptor;

    /// @brief Maximal number of iterations.
    const size_t maxIter;
    /// @brief Desired precision.
    const double eps;
    /// @brief L0 value. Any L0 > 0 fits.
    const double L0;
};

} // namespace gm

