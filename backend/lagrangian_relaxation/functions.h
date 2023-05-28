#pragma once

#include "lr_cppad.h"
#include "log.hpp"

#include <random>
#include <memory>

namespace sea {
namespace backend {

using std::size_t;

// Implemented at: functions.cpp.

double vectorAbsDiffSum(const std::vector<double>& lhs, const std::vector<double>& rhs);

double dualAbsDiffSum(const DualVariables& lhs, const DualVariables& rhs);

double dualL2Norm(const DualVariables& duals);

double dualL1Norm(const DualVariables& duals);

unsigned countNonZeroElements(
        const SubgradientOptimizationParameters& parameters, const double EPS);

// Implemented at: subgradient.cpp.

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

// Implemented at: cutting_plane.cpp.

void updateBest(std::pair<double, DualVariables>& currentBest,
    const DualVariables& candidate, double candidateValue);

DualVariables shuffleDuals(
    const DualVariables& sourceDuals,
    const std::vector<double>& boundLower,
    const std::vector<double>& boundUpper,
    RandomPack* randomPack,
    bool changeMu,
    bool changeLambda,
    bool useUniform,
    bool useNormal);

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

size_t computeTrueCount(const vector<bool>& hitVector);
size_t computeHitDiff(const vector<bool>& prevVector, const vector<bool>& newVector);

pair<size_t, size_t> computeHitStats(const DualTemplate<bool>& currentHit);
pair<size_t, size_t> computeHitChange(
        const DualTemplate<bool>& prevHit, const DualTemplate<bool>& newHit);
void computeBoundHit(const vector<double>& bound,
        const DualVariables& dualVariables, DualTemplate<bool>* hitMap);
vector<bool> computePlaneHits(
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        const vector<double>& lhs,
        const DualVariables& variables);

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

// Implemented at restore.cpp.

void byItineraryRestore(
    const InputData::Event& event,
    const ConstInputManagerPtr& inputManager,
    const ConstLinksManagerPtr& linksManager,
    const ConstLRIndexManagerPtr& indexManager,
    const ConstActionManagerPtr& actionManager,
    const DualVariables& dualVariables,
    State* state,
    DecisionManagerPtr decisionManager);


// Logging methods. Implemented at: log.cpp.
void printDualsToBackendLog(const DualVariables& duals, BackendType type);

void logNonEmptyQInByItineraryRestore(unsigned itineraryId, double nonEmptyQ);
void logEnteredByItineraryRestore(
        const InputData::Event& event, const DualVariables& dualVariables);
void logProcessingItineraryInByItineraryRestore(
        unsigned itineraryId, double lambdaSum, double qr, double zr);
void logObjectiveAndBoundsInByItineraryRestore(const vector<double>& objective,
        const vector<double>& columnLower, const vector<double>& columnUpper);
void logMinCapacityArc(double minCapacityArc);

void logEntranceToCheckIfFeasible(
        const DualVariables& variables,
        const std::vector<double>& boundLower,
        const std::vector<double>& boundUpper,
        const std::vector<double>& lhs);
void logLowerBoundFailureInCheckIfFeasible(double difference);
void logUpperBoundFailureInCheckIfFeasible(double difference);
void logLhsInequalityFailureInCheckIfFeasible(double error);
void logExitFromCheckIfFeasible(const std::string& value);

void logSubgradient(const SubgradientOptimizationParameters& current);

void logHitMuAndLambda(const vector<int>& hitMu, const vector<int>& hitLambda);

void logPlaneIntersectionItineraries(const vector<int>& hitIdx);

void logStartCuttingPlaneOptimization(const State& state, DualVariables* dualPtr);
void logCuttingPlaneIterationDuals(
        unsigned iter, const DualVariables& dualVariables, bool feasible);
void logDualHistorySize(std::size_t historySize);
void logObjectiveAndDualNorms(double cuttingPlaneObjective, double regObjectiveValue,
        const DualVariables& dualVariables, const DualVariables& prevDuals);
void logFinishedCuttingPlaneIteration(unsigned iter, double absObjectiveDiff,
        double absShiftDiff, double relShiftDiff);
void logRestartSimplexFromDeque();
void logLastCuttingPlaneIteration(unsigned iter);
void logChangeInCuttingPlane(
        const std::pair<size_t, size_t>& lowerChange,
        const std::pair<size_t, size_t>& upperChange,
        const std::pair<size_t, size_t>& upperHitCount,
        const std::pair<size_t, size_t>& lowerHitCount,
        size_t targetMuCount,
        size_t targetLambdaCount,
        const vector<bool>& prevPlaneHits,
        const vector<bool>& currentPlaneHits);

void logProcessingImmortalUnchecked();
void logImmortalNorms(const DualDequeInfo& info);
void logImmortalProcessed(bool writeLogOut, const DualDequeInfo& info);

} // namespace backend
} // namespace sea
