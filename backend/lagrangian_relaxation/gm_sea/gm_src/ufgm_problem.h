/**
 * @file ufgm_problem.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements entities for building objectives and constraints for optimization sub-problems
 * in the UFGM. There are two such subproblems: in step 1 and in step 2 according to the lecture 2
 * slides, page 25. The step 1 problem is called Argmin Optimization Problem, and the step 2
 * problem is called Bregman Optimization Problem.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>
#include <numeric>
#include <cppad/ipopt/solve.hpp>

namespace gm {

/**
 * @brief Bregman Optimization problem defines the function
 *  (\xi(v_k, y) + a_{k+1, i_k} * [<\Delta f(x_{k+1, i_k}, y> + \Psi(y))])
 * and constraints y \in Q. This function is
 * required in the optimization step 2 of UFGM.
 */
class BregmanOptimizationProblem {
public:
    /// @brief Simplified notation for CppAD variable.
    typedef std::vector<CppAD::AD<double>> ADvector;

public:
    /**
     * @brief Construct a new Bregman Optimization Problem object.
     *
     * @param grad_f_xk Gradient of the function in point xk.
     * @param ak The coefficient for gradient and regularizer.
     * @param pivot Pivoting point. Corresponds to vk in notation.
     * @param qDescriptor The constraint descriptor, allows to build constraints that force
     * optimization in the projected space.
     * @param regularizer The object to compute regularizer in point.
     * @param prox The object to compute prox function in point.
     * @param adRegularizer The object to compute regularizer derivative in point.
     * @param adProx The object to compute prox functino derivative in point.
     */
    BregmanOptimizationProblem(
            const std::vector<double>& grad_f_xk,
            const double ak,
            const std::vector<double>& pivot,
            std::shared_ptr<const IQDescriptor> qDescriptor,
            std::shared_ptr<const DoubleFunction> regularizer,
            std::shared_ptr<const DoubleFunction> prox,
            std::shared_ptr<const CppADFunction> adRegularizer,
            std::shared_ptr<const CppADFunction> adProx);
    /**
     * @brief Builds optimization objective and constraints.
     *
     * @param functions Objective (located at functions[0]) and constraints (at functions[1:]).
     * @param variables The variable to use for building objective.
     */
    void operator()(ADvector& functions, const ADvector& variables);

private:
    /// @brief Gradient of function f in point xk.
    const std::vector<double> grad_f_xk;
    /// @brief Coefficient for gradient and regularizer.
    const double ak;
    /// @brief Pivoting point, corresponds to vk in notation on slides.
    const std::vector<double> pivot;
    /// @brief Method to build constraints and therefore ensure optimization over projected space.
    std::shared_ptr<const IQDescriptor> qDescriptor;
    /// @brief Object to compute regularizer in point.
    std::shared_ptr<const DoubleFunction> regularizer;
    /// @brief Object to compute prox-function in point.
    std::shared_ptr<const DoubleFunction> prox;
    /// @brief Object to compute regularizer derivative in point.
    std::shared_ptr<const CppADFunction> adRegularizer;
    /// @brief Object to compute prox-function derivative in point.
    std::shared_ptr<const CppADFunction> adProx;
};

/**
 * @brief Argmin Optimization Problem defines the optimization function optimized at the
 * step 1 of UFGM according to slide 25 of Lecture 2. This function is Pxi_k(x) with constraints
 * x \in Q.
 */
class ArgminOptimizationProblem {
public:
    /// @brief Simplified notation for CppAD variable.
    typedef std::vector<CppAD::AD<double>> ADvector;

public:
    /**
     * @brief Construct a new Argmin Optimization Problem object.
     *
     * @param coeffAns Variable linear coefficients in the objective.
     * @param x0 Initial reference point.
     * @param coeffReg Regularizer coefficient in the objective.
     * @param coeffFree Free coefficient in the objective.
     * @param qDescriptor Object for making constraints and therefore enrusing optimization
     * in the projected space.
     * @param regularizer Object to compute regularizer value in point.
     * @param prox Object to compute prox-function value in point.
     * @param adRegularizer Object to compute regularizer derivative in point.
     * @param adProx Object to compute prox-function derivative in point.
     */
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
    /**
     * @brief Builds optimization objective and constraints.
     *
     * @param functions Objective (located at functions[0]) and constraints (at functions[1:]).
     * @param variables The variable to use for building objective.
     */
    void operator()(ADvector& functions, const ADvector& variables);

private:
    /// @brief Variable linear coefficients in the objective.
    const std::vector<double> coeffAns;
    /// @brief Initial point x0.
    const std::vector<double> x0;
    /// @brief Regularizer coefficient in the objective.
    const double coeffReg;
    /// @brief Free coefficient in the objective.
    const double coeffFree;
    /// @brief Object for making constraints and therefore ensuring
    /// optimization in the projected space.
    std::shared_ptr<const IQDescriptor> qDescriptor;
    /// @brief Object to compute regularizer in point.
    std::shared_ptr<const DoubleFunction> regularizer;
    /// @brief Object to compute prox function in point.
    std::shared_ptr<const DoubleFunction> prox;
    /// @brief Object to compute regularizer derivative in point.
    std::shared_ptr<const CppADFunction> adRegularizer;
    /// @brief Object to compute prox-function derivative in point.
    std::shared_ptr<const CppADFunction> adProx;
};

} // namespace gm
