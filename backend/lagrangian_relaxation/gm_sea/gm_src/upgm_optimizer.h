/**
 * @file upgm_optimizer.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implementation for Universal Primal Gradient Method. Defined on slide 17 at
 * lecture 2 from Yuri Nesterov course taught at Higher School of Economics in 2018.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>

#include <cppad/ipopt/solve.hpp>

namespace gm {

/**
 * @brief Implements UPGM optimizer - Universal Primal Gradient Method.
 * UPGM is defined on slide 17 at lecture 2 from Yuri Nesterov course taught at HSE in 2018.
 */
class UniversalPrimalGradientOptimizer : public IGradientOptimizer {
public:
    /**
     * @brief Constructs UPGM entity.
     *
     * @param aTarget The object to compute optimization function value at point.
     * @param aRegularizer The object to compute regularizer value at point.
     * @param aBregmanMapping The object to compute Bregman Mapping.
     * @param aProjector The object to compute projection onto the constraint-defined space.
     * @param aPsiMComputer The object to compute Psi M function.
     * @param anEps Desired precision.
     * @param anL0 L0 initialization. Any number > 0.
     * @param maxIter Maximal number of iterations.
     * @param keepStory Track the evaluation history, if true.
     */
    UniversalPrimalGradientOptimizer(
            std::shared_ptr<DoubleFunction> aTarget,
            std::shared_ptr<DoubleFunction> aRegularizer,
            std::shared_ptr<IBregmanMapping> aBregmanMapping,
            std::shared_ptr<IQProjector> aProjector,
            std::shared_ptr<const IPsiM<double, double>> aPsiMComputer,
            double anEps,
            double anL0,
            size_t maxIter,
            bool keepStory)
        : IGradientOptimizer(keepStory, "UPGM")
        , target(aTarget)
        , regularizer(aRegularizer)
        , projector(aProjector)
        , bregmanMapping(aBregmanMapping)
        , psiMComputer(aPsiMComputer)
        , eps(anEps)
        , L0(anL0)
        , maxIter(maxIter) {}
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~UniversalPrimalGradientOptimizer() {}
    /**
     * @brief Start iterative optimization.
     *
     * @param init Initial point to start from.
     * @return Optimal solution, if converged.
     */
    virtual std::vector<double> optimize(const std::vector<double>& init) const override {
        story.clear();
        auto point = projector->project(init);

        auto bestPoint = point;
        double bestValue = target->getValue(point);

        double Lk = L0;
        bool found = true;
        for (size_t iterId = 0; iterId < maxIter && found; ++iterId) {
            if ((iterId + 1) % 10 == 0) {
                std::cerr << "UPGM: iteration " << iterId + 1 << " " <<  "Lk = " << Lk << " ";
                std::cerr << "bestValue = " << bestValue << std::endl;
            }
            if (target->getValue(point) < bestValue) {
                bestValue = target->getValue(point);
                bestPoint = point;
            }
            double pow2 = 0.5;
            found = false;
            for (size_t i0 = 0; i0 < 128; ++i0) {
                pow2 *= 2;
                auto bregmanX = bregmanMapping->compute(point, pow2 * Lk);
                double psiStar = psiMComputer->getValue(point, bregmanX, pow2 * Lk);
                if (target->getValue(bregmanX)
                        + regularizer->getValue(bregmanX) <= psiStar + eps / 2) {
                    point = bregmanX;
                    Lk = Lk * pow2 / 2;
                    found = true;
                    break;
                }
            }
            if (keepStory) {
                auto curVal = target->getValue(point);
                story["Lk"].push_back(Lk);
                story["current_value"].push_back(curVal);
                story["current_plus_reg_value"].push_back(curVal + regularizer->getValue(point));
                story["current_best"].push_back(bestValue);
            }
            const double BOUND_LK = 1e15;
            if (Lk > BOUND_LK) {
                break;
            }
        }
        return bestPoint;
    }

private:
    /// @brief An object to evaluate the target function at point.
    std::shared_ptr<const DoubleFunction> target;
    /// @brief An object to evaluate regularizer at point.
    std::shared_ptr<const DoubleFunction> regularizer;

    /// @brief An object to project point into constraint-defined space.
    std::shared_ptr<const IQProjector> projector;
    /// @brief An object to compute Bregman Mapping (defined on slide 14 at lecture 2).
    std::shared_ptr<const IBregmanMapping> bregmanMapping;
    /// @brief An object to compute Psi-M function (defined on slide 14 at lecture 2).
    std::shared_ptr<const IPsiM<double, double>> psiMComputer;

    /// @brief Desired precision.
    double eps;
    /// @brief Initial L value. Any number > 0.
    double L0;
    /// @brief Maximal number of iterations.
    size_t maxIter;
};

} // namespace gm
