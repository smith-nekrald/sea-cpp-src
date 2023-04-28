#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>

#include <cppad/ipopt/solve.hpp>


namespace gm {

class UniversalPrimalGradientOptimizer : public IGradientOptimizer {
public:
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
        , maxIter(maxIter) {
    }
    virtual ~UniversalPrimalGradientOptimizer() {}
    virtual std::vector<double> optimize(const std::vector<double>& init) const override {

        story.clear();

        auto point = projector->project(init);

        auto bestPoint = point;
        double bestValue = target->getValue(point);

        double Lk = L0;
	bool found = true;
        for (size_t iter = 0; iter < maxIter && found; ++iter) {
            if ((iter + 1) % 10 == 0) {
                std::cerr << "UPGM: iteration " << iter + 1 << " " <<  "Lk = " << Lk << " ";
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
                story["Lk"].push_back(Lk);
                auto curVal = target->getValue(point);
                story["current_value"].push_back(curVal);
                story["current_plus_reg_value"].push_back(curVal + regularizer->getValue(point));
                story["current_best"].push_back(bestValue);
            }
	    if (Lk > 1e15) {
		    break;
	    }
        }
        return bestPoint;
    }

private:
    std::shared_ptr<const DoubleFunction> target;
    std::shared_ptr<const DoubleFunction> regularizer;

    std::shared_ptr<const IQProjector> projector;
    std::shared_ptr<const IBregmanMapping> bregmanMapping;
    std::shared_ptr<const IPsiM<double, double>> psiMComputer;

    double eps;
    double L0;
    size_t maxIter;
};

} // namespace gm

