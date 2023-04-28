#pragma once

#include "gm_src/gm_interfaces.h"
#include "gm_src/gm_common.h"
#include "gm_src/gm_reg.h"
#include "gm_src/gm_prox.h"
#include "../functions.h"
#include "../lr_cppad.h"
#include "../lr_cppad.hpp"

#include <iostream>

namespace sea {
namespace backend {

struct ManagerStorage {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
    ConstLRIndexManagerPtr indexManager;
    DecisionManagerPtr decisionManager;
};

template<typename Type>
DualTemplate<Type> vector2duals(
        const std::vector<Type>& input,
        const std::size_t muSize,
        const std::size_t lambdaSize) {
    DualTemplate<Type> result;
    result.lambdaVariables.assign(lambdaSize, Type(0));
    result.muVariables.assign(muSize, Type(0));
    assert(input.size() == muSize + lambdaSize);
    for (std::size_t idx = 0; idx < input.size(); ++idx) {
        if (idx < lambdaSize) {
            result.lambdaVariables[idx] = input[idx];
        } else {
            result.muVariables[idx - lambdaSize] = input[idx];
        }
    }
    return result;
}

template<typename Type>
std::vector<Type> duals2vector(
        const DualTemplate<Type>& duals,
        std::size_t* muSize,
        std::size_t* lambdaSize) {
    assert(muSize != nullptr);
    assert(lambdaSize != nullptr);
    *muSize = duals.muVariables.size();
    *lambdaSize = duals.lambdaVariables.size();
    std::vector<Type> result = duals.lambdaVariables;
    for (const auto& item : duals.muVariables) {
        result.push_back(item);
    }
    return result;
}

template<typename Type>
class SeaObjective : public gm::IOrderOneOracle<Type> {
public:
    SeaObjective(
            ManagerStorage storage,
            const State& state,
            bool ignoreSpot)
        : storage(storage)
        , state(state)
        , ignoreSpot(ignoreSpot)
        , muSize(storage.indexManager->getConstData().muCount)
        , lambdaSize(storage.indexManager->getConstData().lambdaCount)
    {};
    virtual Type getValue(const std::vector<Type>& point) const {
        assert(point.size() == muSize + lambdaSize);
        auto duals = vector2duals<Type>(point, muSize, lambdaSize);
        return computeFunctionValue<Type>(
                storage.inputManager->getConstData(),
                storage.linksManager->getConstData(),
                state,
                storage.indexManager->getConstData(),
                state.timeParameters,
                storage.decisionManager->get(),
                duals, ignoreSpot, 0.);
    }
    virtual std::vector<Type> getSubgradient(const std::vector<Type>& domain) const {
        auto adPoint = gm::makeADRoutine(domain);
        auto duals = vector2duals<CppAD::AD<Type>>(adPoint, muSize, lambdaSize);
        vector<CppAD::AD<Type>> objective = {
            computeFunctionValue<CppAD::AD<Type>>(
                storage.inputManager->getConstData(),
                storage.linksManager->getConstData(),
                state,
                storage.indexManager->getConstData(),
                state.timeParameters,
                storage.decisionManager->get(),
                duals, ignoreSpot, 0.)
        };
        return gm::subgradientRoutine<Type>(domain, adPoint, objective);
    }
    virtual ~SeaObjective() {};

private:
    mutable ManagerStorage storage;
    const State& state;
    const bool ignoreSpot;
    std::size_t muSize;
    std::size_t lambdaSize;

};

void doGMOptimization(
        const ConstInputManagerPtr& inputManager,
        const ConstLinksManagerPtr& linksManager,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& configIter,
        const State& state,
        DecisionManagerPtr& decisionManager,
        RandomPack* randomPack,
        DualVariables* dualPtr,
        std::deque<DualDequeInfo>* dualHistory,
        double* estimatedOjective,
        UCoefficients* uCoeff,
        bool ignoreSpot,
        DualVariables mean);


} // namespace backend
} //namespace sea
