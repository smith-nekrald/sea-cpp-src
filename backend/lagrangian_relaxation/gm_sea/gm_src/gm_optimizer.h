#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>

#include <cppad/ipopt/solve.hpp>


namespace gm {

class GradientOptimizer : public IGradientOptimizer {
public:
    GradientOptimizer(std::shared_ptr<DoubleFunction> aTarget,
            std::shared_ptr<DoubleFunction> aRegularizer,
            std::shared_ptr<IQProjector> aProjector,
            double aStep, size_t aMaxIter, bool keepStory)
        : IGradientOptimizer(keepStory, "GradientOptimizer")
        , target(aTarget)
        , regularizer(aRegularizer)
        , projector(aProjector)
        , maxIter(aMaxIter)
        , step(aStep) {
    };
    virtual std::vector<double> optimize(const std::vector<double>& init) const {
        story.clear();

        std::vector<double> point = projector->project(init);
        std::vector<double> bestPoint = init;
        double bestValue = target->getValue(point);

        for (size_t iter = 0; iter < maxIter; ++iter) {
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
    virtual ~GradientOptimizer() {};
private:
    std::shared_ptr<DoubleFunction> target;
    std::shared_ptr<DoubleFunction> regularizer;
    std::shared_ptr<IQProjector> projector;
    size_t maxIter;
    double step;
};

} // namespace gm

