/**
 * @file sea_gm_apply.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements API for bridging Sea Cargo objective induced by Lagrangian Relaxation
 * and gradient optimization methods.
 * @copyright (c) Smith School of Business, 2023
 */
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

/**
 * @brief Storage for all relevant managers.
 */
struct ManagerStorage {
    /// @brief Manager with InputData.
    ConstInputManagerPtr inputManager;
    /// @brief Manager with InputLinks.
    ConstLinksManagerPtr linksManager;
    /// @brief Manager with LR Index.
    ConstLRIndexManagerPtr indexManager;
    /// @brief Manager with Decision.
    DecisionManagerPtr decisionManager;
};

/**
 * @brief Converts 1-dimensional vector to DualVariables. Gradient optimization works with
 * universal standard vectors, and this method is required to convert into LR-understandable
 * format.
 * 
 * @tparam Type The type of vector entry.
 * @param input The vector to convert.
 * @param muSize The size of mu variables vector.
 * @param lambdaSize The size of lambda variables vector.
 * @return DualVariables, initialized from input vector.
 */
template<typename Type>
DualTemplate<Type> vector2duals(
        const std::vector<Type>& input, const std::size_t muSize, const std::size_t lambdaSize) {
    DualTemplate<Type> result;
    result.lambdaVariables.assign(lambdaSize, static_cast<Type>(0));
    result.muVariables.assign(muSize, static_cast<Type>(0));
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

/**
 * @brief Converts DualVariables to vector. Gradient optimization works with universal standard 
 * vectors, and this method is required to convert from innder LR-understandable format to format 
 * understandable by gradient optimizers..
 * 
 * @tparam Type The type of vector entry.
 * @param duals The object representing duals.
 * @param muSize The number of mu varaibles.
 * @param lambdaSize The number of lambda variables.
 * @return The vector built from duals.
 */
template<typename Type>
std::vector<Type> duals2vector(
        const DualTemplate<Type>& duals, std::size_t* muSize, std::size_t* lambdaSize) {
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

/**
 * @brief Constructs objective for sea cargo LR-based optimization in the format, compatible
 * with gradient optimization methods.
 * 
 * @tparam Type The point type. Expected options are double and CppAD<double>.
 */
template<typename Type>
class SeaObjective : public gm::IOrderOneOracle<Type> {
public:
    /**
     * @brief SeaObjective constructor.
     * 
     * @param storage Storage with all relevant managers.
     * @param state The state of evaluation trajectory.
     * @param ignoreSpot If true, spot market is ignored.
     */
    SeaObjective(ManagerStorage storage, const State& state, bool ignoreSpot)
        : storage(storage)
        , state(state)
        , ignoreSpot(ignoreSpot)
        , muSize(storage.indexManager->getConstData().muCount)
        , lambdaSize(storage.indexManager->getConstData().lambdaCount)
    {};
    /**
     * @brief Compute/Express function value at point.
     * 
     * @param point The point to compute/express function value at.
     * @return The computed/expressed function value.
     */
    virtual Type getValue(const std::vector<Type>& point) const {
        assert(point.size() == muSize + lambdaSize);
        auto duals = vector2duals<Type>(point, muSize, lambdaSize);
        return computeFunctionValue<Type>(
                storage.inputManager->getConstData(), storage.linksManager->getConstData(), state,
                storage.indexManager->getConstData(), state.timeParameters, 
                storage.decisionManager->get(), duals, ignoreSpot, 0.);
    }
    /**
     * @brief Compute/express function subgradient in point.
     * 
     * @param domain The point to compute/express subgradient at.
     * @return The computed/expressed subgradient at point.
     */
    virtual std::vector<Type> getSubgradient(const std::vector<Type>& domain) const {
        auto adPoint = gm::makeADRoutine(domain);
        auto duals = vector2duals<CppAD::AD<Type>>(adPoint, muSize, lambdaSize);
        vector<CppAD::AD<Type>> objective = {
            computeFunctionValue<CppAD::AD<Type>>(
                storage.inputManager->getConstData(), storage.linksManager->getConstData(), state,
                storage.indexManager->getConstData(), state.timeParameters, 
                storage.decisionManager->get(), duals, ignoreSpot, 0.)
        };
        return gm::subgradientRoutine<Type>(domain, adPoint, objective);
    }
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~SeaObjective() {};

private:
    /// @brief The storage with all relevant managers.
    mutable ManagerStorage storage;
    /// @brief State of evaluation trajectory.
    const State& state;
    /// @brief If true, spot market is ignored.
    const bool ignoreSpot;
    /// @brief Number of mu dual variables.
    std::size_t muSize;
    /// @brief Number of lambda dual variables.
    std::size_t lambdaSize;

};

/**
 * @brief Launches Gradient Method optimization. Converts Sea Cargo lagrangian relaxation API
 * into a form compatible with gradient methods and launches a relevant subgradient optimizator.
 * 
 * @param[in] inputManager The Manager with InputData.
 * @param[in] linksManager The Manager with InputLinsk.
 * @param[in] indexManager The Manager with Lagrangian Relaxation index.
 * @param[in] configIter Configuration for the Lagrangian Relaxation backend.
 * @param[in] state The state of trajectory evaluation.
 * @param[out] decisionManager The Manager with Decision.
 * @param[out] randomPack Randomization package.
 * @param[out] dualPtr Pointer to dual variables. Those are optimized. 
 * @param[out] dualHistory History with dual variables. Ignored in this realization.
 * @param[out] estimatedOjective Pointer for writing objective estimation.
 * @param[out] uCoeff Pointer to u-coefficients.
 * @param[in] ignoreSpot  If true, spot market is ignored.
 * @param[in] mean Center for building effective regularizer.
 */
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
