#include "sea_gm_apply.h"
#include "sea_constraints.h"

#include "../lr_cppad.hpp"
#include "../lagrangian_relaxation_backend.h"
#include "../../../logging/logging.h"
#include "../functions.h"

#include "gm_src/gm_interfaces.h"
#include "gm_src/ufgm_optimizer.h"
#include "gm_src/upgm_optimizer.h"
#include "gm_src/gm_optimizer.h"
#include "gm_src/gm_types.h"
#include "gm_src/gm_projector.h"
#include "gm_src/gm_bregman.h"

#include <limits>
#include <functional>
#include <cstdio>
#include <numeric>
#include <memory>
#include <fstream>

namespace sea {
namespace backend {

using namespace gm;

void doGMOptimization(
        const ConstInputManagerPtr& inputManager,
        const ConstLinksManagerPtr& linksManager,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& configIter,
        const State& state,
        DecisionManagerPtr& decisionManager,
        RandomPack* randomPack,
        DualVariables* dualPtr,
        std::deque<DualDequeInfo>* /*dualHistory*/,
        double* estimatedObjective,
        UCoefficients* uCoeff,
        bool ignoreSpot,
        DualVariables mean) {

    assert(dualPtr != nullptr);
    std::vector<double> meanVector;
    if (mean.lambdaVariables.size() && mean.muVariables.size()) {
        std::size_t muSizeTmp = 0, lambdaSizeTmp = 0;
        meanVector = duals2vector<double>(mean, &muSizeTmp, &lambdaSizeTmp);
    }

    ManagerStorage storage = {inputManager, linksManager, indexManager, decisionManager};
    std::shared_ptr<IQDescriptor> qDescriptor = std::make_shared<SeaQDescriptor>(
        storage, state, configIter);

    std::vector<double> lower, upper, lhs;
    qDescriptor->initBoundsLR(&lower, &upper);

    auto init = provideSomeSolution(
            lower, upper, lhs, configIter.initStrategy.value(),
            inputManager, indexManager, configIter, randomPack, true, true);

    std::shared_ptr<DoubleFunction> target =
        std::make_shared<SeaObjective<double>>(storage, state, ignoreSpot);
    std::shared_ptr<CppADFunction> adTarget =
        std::make_shared<SeaObjective<CppAD::AD<double>>>(storage, state, ignoreSpot);

    std::shared_ptr<DoubleFunction> regularizer = nullptr;
    std::shared_ptr<CppADFunction> adRegularizer = nullptr;
    if (configIter.ufgmRegType == GmRegularizerType::L2SQUARED) {
        regularizer = std::make_shared<L2SquaredReg<double>>(
                configIter.coeffReg.value(), meanVector);
        adRegularizer = std::make_shared<L2SquaredReg<CppAD::AD<double>>>(
                configIter.coeffReg.value(), meanVector);
    } else {
        throw std::logic_error("Unknown regularizer type!");
    }

    std::shared_ptr<DoubleFunction> prox = nullptr;
    std::shared_ptr<CppADFunction> adProx = nullptr;

    if (configIter.ufgmProxType == GmProxType::L2SQUARED) {
        prox = std::make_shared<ProxL2Squared<double>>();
        adProx = std::make_shared<ProxL2Squared<CppAD::AD<double>>>();
    } else {
        throw std::logic_error("Unknown prox type!");
    }
    std::shared_ptr<IGradientOptimizer> optimizer = nullptr;
    std::shared_ptr<IQProjector> projector = std::make_shared<MinDistProjector>(qDescriptor);
    std::shared_ptr<IBregmanMapping> bregman = std::make_shared<BregmanMapping>(
        target, adRegularizer, prox, adProx, qDescriptor);
    std::shared_ptr<BregmanDistance<double, double>> distBreg = std::make_shared<
        BregmanDistance<double, double>>(prox, prox);
    std::shared_ptr<IPsiM<double, double>> psi = std::make_shared<PsiM<double, double>>(
            target, regularizer, distBreg);
    if (configIter.gradientOptimizer == GradientFamily::UFGM) {
        optimizer = std::make_shared<UFGMOptimizer>(target, regularizer, prox,
            adTarget, adRegularizer, adProx, qDescriptor,
            configIter.maxSubgradientIterations.value(),
            configIter.subgradientPrecision.value(),
            configIter.L0.value(),
            configIter.keepStory.value());
    } else if (configIter.gradientOptimizer == GradientFamily::GM) {
        optimizer = std::make_shared<GradientOptimizer>(
                target, regularizer,
                projector, 1. / configIter.L0.value(),
                configIter.maxSubgradientIterations.value(),
                configIter.keepStory.value());
    } else if (configIter.gradientOptimizer == GradientFamily::UPGM) {
        optimizer = std::make_shared<UniversalPrimalGradientOptimizer>(
                target, regularizer, bregman, projector, psi,
                configIter.subgradientPrecision.value(),
                configIter.L0.value(),
                configIter.maxSubgradientIterations.value(),
                configIter.keepStory.value());
    } else {
        throw std::runtime_error("Unknown GradientFamily optimizer!");
    }
    std::size_t muSize = 0, lambdaSize = 0;
    std::vector<double> initVector = duals2vector<double>(init, &muSize, &lambdaSize);
    initVector = projector->project(initVector);
    auto resultVector = optimizer->optimize(initVector);
    auto resultDuals = vector2duals(resultVector, muSize, lambdaSize);
    *dualPtr = resultDuals;
    if (estimatedObjective != nullptr) {
        *estimatedObjective = target->getValue(resultVector);
    }
    if (uCoeff != nullptr) {
        computeSubgradientInPoint(
                resultDuals, state, linksManager, inputManager,
                indexManager, decisionManager, uCoeff, ignoreSpot);
    }
    if (configIter.keepStory.value()) {
        auto story = optimizer->getStory();
        printStory(story, "gm", optimizer->getName());
    }
}


} // namespace backend
} // namespace sea
