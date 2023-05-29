/**
 * @file gm_optimizer.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements canonical gradient step optimization.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>

#include <cppad/ipopt/solve.hpp>


namespace gm {

/**
 * @brief Canonical gradient descent step optimizer.
 */
class GradientOptimizer : public IGradientOptimizer {
public:
    /**
     * @brief Constructs Canonical Gradient Descent Optimizer.
     * 
     * @param aTarget Object for computing target function value and subgradient in point.
     * @param aRegularizer Object for computing regularizer value and subgradient in point.
     * @param aProjector Object for projecting onto domain space.
     * @param aStep Gradient Step.
     * @param aMaxIter Maximal number of iteration.
     * @param keepStory If to keep measurments history.
     */
    GradientOptimizer(std::shared_ptr<DoubleFunction> aTarget,
            std::shared_ptr<DoubleFunction> aRegularizer,
            std::shared_ptr<IQProjector> aProjector,
            double aStep, size_t aMaxIter, bool keepStory)
        : IGradientOptimizer(keepStory, "GradientOptimizer")
        , target(aTarget)
        , regularizer(aRegularizer)
        , projector(aProjector)
        , maxIter(aMaxIter)
        , step(aStep) {};

    /**
     * @brief Iteratively applies gradient optimization.
     * 
     * @param init Initial point to start from.
     * @return If converged, optimal point. Otherwise, point after maxIter steps.
     */
    virtual std::vector<double> optimize(const std::vector<double>& init) const {
        story.clear();
        std::vector<double> point = projector->project(init);
        std::vector<double> bestPoint = init;
        double bestValue = target->getValue(point);
        for (size_t iterId = 0; iterId < maxIter; ++iterId) {
            auto subGradient = target->getSubgradient(point);
            auto regGradient = regularizer->getSubgradient(point);
            for (size_t idx = 0; idx < point.size(); ++idx) {
                point[idx] = point[idx] - step * (subGradient[idx] + regGradient[idx]);
            }
            point = projector->project(point);
            double value = target->getValue(point);
            if (value < bestValue) {
                bestValue = value;
                bestPoint = point;
            }
            if (keepStory) {
                story["best_value"].push_back(bestValue);
                story["value"].push_back(value);
            }
        }
        return bestPoint;
    };

    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~GradientOptimizer() {};

private:
    /// @brief Object to compute function value and subgradient in point.
    std::shared_ptr<DoubleFunction> target;
    /// @brief Object to compute regularizer value and subgradient in point.
    std::shared_ptr<DoubleFunction> regularizer;
    /// @brief Object for projecting onto domain subspace.
    std::shared_ptr<IQProjector> projector;
    /// @brief The maximal number of iterations.
    size_t maxIter;
    /// @brief The gradient step size.
    double step;
};

} // namespace gm

