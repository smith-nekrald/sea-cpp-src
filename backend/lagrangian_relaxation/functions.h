#pragma once

#include "lr_cppad.h"

#include <random>
#include <memory>

namespace sea {
namespace backend {

double dualL2Norm(const DualVariables& duals);
double dualL1Norm(const DualVariables& duals);

SubgradientOptimizationParameters computeSubgradientInPoint(
        const DualVariables& point,
        const State& state,
        const ConstLinksManagerPtr& linksManager,
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        DecisionManagerPtr decisionManager,
        UCoefficients* coeffU = nullptr,
        bool ignoreSpot = false,
        double l2CoeffReg = 0,
        DualVariables mean = DualVariables());

bool checkIfFeasible(
        const DualVariables& variables,
        const std::vector<double>& boundLower,
        const std::vector<double>& boundUpper,
        const std::vector<double>& lhs,
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& configIter);

void processMinValue(
        const std::vector<double> lower,
        const std::vector<double> upper,
        DualVariables* target);

void processAvgValue(
        const std::vector<double> lower,
        const std::vector<double> upper,
        DualVariables* target);

void transformAdd(
        std::bernoulli_distribution& selector,
        std::default_random_engine& generator,
        double addValue,
        DualVariables* target);

DualVariables provideSomeSolution(
        const std::vector<double>& boundLower,
        const std::vector<double>& boundUpper,
        const std::vector<double>& lhs,
        const RandomInitType& initType,
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& config,
        RandomPack* randomPack,
        bool minInit,
        double lambdaAdd);

void addDualsToSimplex(
        const DualVariables& point,
        unsigned ncols,
        const LagrangianRelaxationBackendConfig& configIter,
        const State& state,
        const ConstLinksManagerPtr& linksManager,
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        DecisionManagerPtr decisionManager,
        ClpSimplex* simplexBase,
        bool ignoreSpot = false,
        double* objectiveValue = nullptr,
        DualVariables mean = DualVariables());

void doCuttingPlaneOptimization(
    const ConstInputManagerPtr& inputManager,
    const ConstLinksManagerPtr& linksManager,
    const ConstLRIndexManagerPtr& indexManager,
    const LagrangianRelaxationBackendConfig& configIter,
    const State& state,
    DecisionManagerPtr& decisionManager,
    RandomPack* randomPack,
    DualVariables* dualPtr,
    std::deque<DualDequeInfo>* dualHistory,
    double* estimatedObjective = nullptr,
    UCoefficients* uCoeff = nullptr,
    bool ignoreSpot = false,
    DualVariables mean = DualVariables());

unsigned countNonZeroElements(const SubgradientOptimizationParameters& parameters, const double EPS);

double dualAbsDiffSum(const DualVariables& lhs, const DualVariables& rhs);

double vectorAbsDiffSum(const std::vector<double>& lhs, const std::vector<double>& rhs);

void byItineraryRestore(
    const InputData::Event& event,
    const ConstInputManagerPtr& inputManager,
    const ConstLinksManagerPtr& linksManager,
    const ConstLRIndexManagerPtr& indexManager,
    const ConstActionManagerPtr& actionManager,
    const DualVariables& dualVariables,
    State* state,
    DecisionManagerPtr decisionManager);

void printDualsToBackendLog(const DualVariables& duals, BackendType type);

template<typename T>
void printArrayToBackendLog(const std::vector<T> array,
        BackendType type) {
    auto stream = logging::getBackendSubLogger(type) << log4cpp::Priority::DEBUG;
    stream << " Array: ";
    for (const auto& value : array) {
        stream << value << " ";
    }
}

std::pair<double, DualVariables> initializeCuttingPlane(
    const ConstInputManagerPtr& inputManager,
    const ConstLinksManagerPtr& linksManager,
    const ConstLRIndexManagerPtr& indexManager,
    const LagrangianRelaxationBackendConfig& configIter,
    const State& state,
    const vector<double>& columnLower,
    const vector<double>& columnUpper,
    const vector<double>& lhsArray,
    unsigned ncols,
    RandomPack* randomPack,
    std::deque<DualDequeInfo>* dualHistory,
    ClpSimplex& simplexBase,
    DecisionManagerPtr& decisionManager,
    bool ignoreSpot,
    DualVariables mean);

void prepareSimplex(
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& configIter,
        DualVariables* dualPtr,
        ClpSimplex* simplexBasePtr,
        vector<double>* columnLowerPtr,
        vector<double>* columnUpperPtr,
        vector<double>* lhsArrayPtr,
        unsigned* ncolsPtr);

} // namespace backend
} // namespace sea
