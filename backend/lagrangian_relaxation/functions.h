/**
 * @file functions.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares cutting plane functions.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "lr_cppad.h"
#include "log.hpp"

#include <random>
#include <memory>

namespace sea {
namespace backend {

using std::size_t;

// Implemented at: functions.cpp.

/**
 * @brief Absolute value sum of vector pointwise difference. L1 norm.
 * 
 * @param lhs The first vector.
 * @param rhs The second vector.
 * @return The L1 norm value.
 */
double vectorAbsDiffSum(const std::vector<double>& lhs, const std::vector<double>& rhs);

/**
 * @brief Sum of absolute values of pointwise differences for two DualVariables points.
 * Essentially, L1_norm(mu_1 - mu_2) + L1_norm(lambda_1 - lambda_2).
 * 
 * @param lhs The first DualVariables point.
 * @param rhs The second DualVariables point.
 * @return The computed value.
 */
double dualAbsDiffSum(const DualVariables& lhs, const DualVariables& rhs);

/**
 * @brief L2 norm of a DualVariables point. Formally, sqrt(sum(mu**2) + sum(lamba**2)).
 * 
 * @param duals The DualVariables point to compute norm at.
 * @return The computed norm.
 */
double dualL2Norm(const DualVariables& duals);

/**
 * @brief L1 norm of a DualVariables point. Formally, sum(abs(mu)) + sum(abs(lambda)).
 * 
 * @param duals The DualVariables point to compute norm at.
 * @return The computed norm.
 */
double dualL1Norm(const DualVariables& duals);
/**
 * @brief Counts the number of substantially different from zero elements in subgradient.
 * 
 * @param parameters The Subgradient to count the desired number at.
 * @param EPS The minimal difference to tread two floating point numbers as different.
 * @return The number of substantially different from zero elements in subgradient.
 */
unsigned countNonZeroElements(
        const SubgradientOptimizationParameters& parameters, const double EPS);

// Implemented at: subgradient.cpp.

/**
 * @brief Computes LR objective subgradient in point.
 * 
 * @param[in] point The point to compute subgradient at.
 * @param[in] state The state of trajectory evaluation.
 * @param[in] linksManager Manager with InputLinks.
 * @param[in] inputManager Manager with InputData.
 * @param[in] indexManager Manager with Lagrangian Relaxation Index.
 * @param[out] decisionManager Manager with Decision.
 * @param[out] coeffU Allotment u-coefficients (helps in Benders Decomposition).
 * @param[in] ignoreSpot If true, spot market is ignored.
 * @param[in] l2CoeffReg The regularization coefficient.
 * @param[in] mean The mean dual value for effective centering.
 * @return The computed subgradient.
 */
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

/**
 * @brief Updates best DualVariable point if new objective is smaller.
 * 
 * @param[out] currentBest The current value of the objective and where this objective is obtained.
 * @param[in] candidate Candidate point.
 * @param[in] candidateValue Candidate objective value. 
 */
void updateBest(std::pair<double, DualVariables>& currentBest,
    const DualVariables& candidate, double candidateValue);

/**
 * @brief Randomly disturbed dual variables.
 * 
 * @param[in] sourceDuals The reference dual variables to modify.
 * @param[in] boundLower Lower bounds.
 * @param[in] boundUpper Upper bounds.
 * @param[out] randomPack Pack with randomness.
 * @param[in] changeMu If to change mu duals.
 * @param[in] changeLambda If to change lambda duals.
 * @param[in] useUniform If true, uniform distrubution disturbing is applied.
 * @param[in] useNormal If true, normal distribution disturbing is applied.
 * @return The randomly disturbed sourceDuals.
 */
DualVariables shuffleDuals(
    const DualVariables& sourceDuals,
    const std::vector<double>& boundLower,
    const std::vector<double>& boundUpper,
    RandomPack* randomPack,
    bool changeMu,
    bool changeLambda,
    bool useUniform,
    bool useNormal);

/**
 * @brief Checks if a DualVariables point is feasible.
 * 
 * @param variables Variables to verify.
 * @param boundLower Vector with lower bounds.
 * @param boundUpper Vector with upper bounds.
 * @param lhs Vector with constraints LHS.
 * @param inputManager Manager with Input Data.
 * @param indexManager Manager with LR Index.
 * @param configIter LR Backend configuration.
 * @return If the point is feasible, returns true. Otherwise returns false.
 */
bool checkIfFeasible(
    const DualVariables& variables,
    const std::vector<double>& boundLower,
    const std::vector<double>& boundUpper,
    const std::vector<double>& lhs,
    const ConstInputManagerPtr& inputManager,
    const ConstLRIndexManagerPtr& indexManager,
    const LagrangianRelaxationBackendConfig& configIter);

/**
 * @brief Sets muVariables to lower and adjusts lambdaVariables accordingly.
 * 
 * @param[in] lower Lower values for mu duals.
 * @param[in] upper Upper values for mu duals.
 * @param[out] target The target duals to process.
 */
void processMinValue(
    const std::vector<double>& lower, const std::vector<double>& upper, DualVariables* target);

/**
 * @brief Sets muVariables to (lower + upper)/2 and adjusts lambdaVariables accordingly.
 * 
 * @param[in] lower Lower values range for mu duals.
 * @param[in] upper Upper values range for mu duals.
 * @param[out] target The target duals to process.
 */
void processAvgValue(
    const std::vector<double>& lower, const std::vector<double>& upper, DualVariables* target);

/**
 * @brief Randomly adds to each lambda duals addValue if bernoulli distribution generates 1.
 * Yet another way to disturb DualVariables point.
 * 
 * @param[out] selector The Bernoulli distribution selector.
 * @param[out] generator The random engine to turn on the selector.
 * @param[in] addValue The value to add.
 * @param[out] target The DualVariables to update lambda duals.
 */
void transformAdd(
    std::bernoulli_distribution& selector,
    std::default_random_engine& generator,
    double addValue,
    DualVariables* target);

/**
 * @brief Method to generate some DualVariables solution point.
 * 
 * @param[in] boundLower Lower bound on solution point, entry-wise.
 * @param[in] boundUpper Upper bound on solution point, entry-wise.
 * @param[in] lhs LHS value for inequalities to fit.
 * @param[in] initType Type of random initialization.
 * @param[in] inputManager Manager with InputData.
 * @param[in] indexManager Manager with LR Index.
 * @param[in] config Configuration for LR Backend.
 * @param[out] randomPack The package with randomness.
 * @param[in] minInit Whether to init with minimal values.
 * @param[in] lambdaAdd Parameter to add to lambda variables for Bernoulli transformations.
 * @return Generated DualVariables point.
 */
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

/**
 * @brief Adds new dual variable point to simplex.
 * 
 * @param[in] point The point to add.
 * @param[in] ncols Number of columns.
 * @param[in] configIter LR backend configuration.
 * @param[in] state Evaluation trajectory state.
 * @param[in] linksManager Manager with InputLinks.
 * @param[in] inputManager Manager with InputData.
 * @param[in] indexManager Manager with LR Index.
 * @param[out] decisionManager Manager with Decision.
 * @param[out] simplexBase Simplex to add the point to.
 * @param[in] ignoreSpot If true, spot market is ignored.
 * @param[out] objectiveValue If not null, expects to get assigned to estimated objective.
 * @param[in] mean Mean duals for centering.
 */
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

/**
 * @brief Prepares simplex for optimization.
 * 
 * @param[in] inputManager Manager with Input Data.
 * @param[in] indexManager Manager with LR Index.
 * @param[in] configIter LR Backend configuration.
 * @param[in] dualPtr Point with dual variables. Starting point.
 * @param[out] simplexBasePtr Simplex Base to update.
 * @param[out] columnLowerPtr Column Lower bounds.
 * @param[out] columnUpperPtr Column Upper bounds.
 * @param[out] lhsArrayPtr LHS array.
 * @param[out] ncolsPtr Number of columns.
 */
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

/**
 * @brief Ititializes cutting plane optimization. 
 * Generates certain amount of points to start with.
 * Also adds points from dual history to simplex. 
 * 
 * @param[in] inputManager Manager with InputData.
 * @param[in] linksManager Manager with InputLinks.
 * @param[in] indexManager Manager with LR Index.
 * @param[in] configIter LR Backend configuration.
 * @param[in] state State of trajectory evaluation.
 * @param[in] columnLower Lower column bounds.
 * @param[in] columnUpper Upper column bounds.
 * @param[in] lhsArray LHS constraint array.
 * @param[in] ncols Number of columns
 * @param[out] randomPack Package with randomness.
 * @param[out] dualHistory Dual variables history.
 * @param[out] simplexBase Simplex base.
 * @param[out] decisionManager Manager with decision.
 * @param[in] ignoreSpot If true, spot market is ignored.
 * @param[in] mean Mean dual variable value for centering.
 * @return Currently best solution. Pair with value and point obtaining that value.
 */
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

/**
 * @brief Computes amount of true values in boolean hitVector.
 * 
 * @param hitVector The vector to count the true values at.
 * @return The number of entries in hitVector which are true.
 */
size_t computeTrueCount(const vector<bool>& hitVector);
/**
 * @brief Computes the number of places where boolean vectors are different.
 * 
 * @param prevVector The first boolean vector.
 * @param newVector The second boolean vector.
 * @return The number of entries at which the boolean vectors are different.
 */
size_t computeHitDiff(const vector<bool>& prevVector, const vector<bool>& newVector);

/**
 * @brief Computes the number of true entries in mu-part and lambda-part.
 * 
 * @param currentHit The template with mu-part and lambda part.
 * @return The pair with two entries. The first entry is the number of true values in lambda-part,
 * the second entry is the number of true values in mu-part.
 */
pair<size_t, size_t> computeHitStats(const DualTemplate<bool>& currentHit);
/**
 * @brief Computes the number of times when boolean entries 
 * in mu-part and lambda-part are different.
 * 
 * @param prevHit The first boolean dual template with mu-part and lambda-part.
 * @param newHit The second boolean dual tempalte with mu-part an lambda-part.
 * @return The pair with two entries. The first entry is the number of times when lambda parts
 * are different, the second entry is the number of times when mu parts are different.
 */
pair<size_t, size_t> computeHitChange(
        const DualTemplate<bool>& prevHit, const DualTemplate<bool>& newHit);
/**
 * @brief Studies when dual variables point hits bound, and stores result in hitMap.
 * 
 * @param[in] bound The bound that gets hit.
 * @param[in] dualVariables The dual variables that may hit the bound.
 * @param[out] hitMap If corresponding dual variable hits the bound, the corresponding entry
 * is set to true. And false otherwise.
 */
void computeBoundHit(const vector<double>& bound,
        const DualVariables& dualVariables, DualTemplate<bool>* hitMap);
/**
 * @brief Studies when dual variables point hits itinerary-induced plane.
 * 
 * @param inputManager The Manager with InputData.
 * @param indexManager The Manager with LR Index.
 * @param lhs LHS for itinerary-induced planes.
 * @param variables The dual variable point to study.
 * @return Vector with boolean entries, entry idx is true if and only if dual variables point
 * hits itinerary-induced plane with index idx.
 */
vector<bool> computePlaneHits(
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        const vector<double>& lhs,
        const DualVariables& variables);

/**
 * @brief Performs cutting plane optimization.
 * 
 * @param[in] inputManager Manager with Input Data.
 * @param[in] linksManager Manager with Input Links.
 * @param[in] indexManager Manager with LR Index.
 * @param[in] configIter Configuration for LR Optimization.
 * @param[in] state The trajectory evaluation state.
 * @param[out] decisionManager The Manager with Decision.
 * @param[out] randomPack 
 * @param[out] dualPtr 
 * @param[out] dualHistory Dual variable history.
 * @param[out] estimatedObjective Objective estimation, when requested.
 * @param[out] uCoeff Allotment u-coefficients for Benders Decomposition, when requested.
 * @param[in] ignoreSpot If true, spot market is ignored.
 * @param[in] mean Mean dual variable value for centering.
 */
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

/**
 * @brief Restores cutoff event decision from known estimated dual variables.
 * 
 * @param[in] event The event to process.
 * @param[in] inputManager Manager with InputData.
 * @param[in] linksManager Manager with InputLinks.
 * @param[in] indexManager Manager with LR index.
 * @param[in] actionManager Manager with Action.
 * @param[in] dualVariables Dual variables to use for making a decision.
 * @param[out] state The state of trajectory evaluation.
 * @param[out] decisionManager The Manager for Decision.
 */
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

/**
 * @brief Prints dual variables to backend log.
 * 
 * @param duals The vector with dual variables to print.
 * @param type Type of backend for logger creation.
 */
void printDualsToBackendLog(const DualVariables& duals, BackendType type);

/**
 * @brief Logs amount of non-empty containers loaded in byItineraryRestore.
 * 
 * @param itineraryId The index of itinerary.
 * @param nonEmptyQ The amount of loaded non-empty containers.
 */
void logNonEmptyQInByItineraryRestore(unsigned itineraryId, double nonEmptyQ);
/**
 * @brief Logs that byItineraryRestore is started.
 * 
 * @param event The current event.
 * @param dualVariables The current dual variables.
 */
void logEnteredByItineraryRestore(
        const InputData::Event& event, const DualVariables& dualVariables);
/**
 * @brief Logs processing fixed itinerary in byItineraryRestore.
 * 
 * @param itineraryId Index of processed itinerary.
 * @param lambdaSum Sum of the corresponding lambda duals.
 * @param qr The qr value.
 * @param zr The zr value.
 */
void logProcessingItineraryInByItineraryRestore(
        unsigned itineraryId, double lambdaSum, double qr, double zr);
/**
 * @brief Logs objective and bounds in byItineraryRestore.
 * 
 * @param objective The objective coefficients.
 * @param columnLower The vector with lower bounds.
 * @param columnUpper The vector with upper bounds.
 */
void logObjectiveAndBoundsInByItineraryRestore(const vector<double>& objective,
        const vector<double>& columnLower, const vector<double>& columnUpper);
/**
 * @brief Logs the minimal arc capacity.
 * 
 * @param minCapacityArc The minimal capacity.
 */
void logMinCapacityArc(double minCapacityArc);

/**
 * @brief Logs that checkIfFeasible is started.
 * 
 * @param variables The variables to check on feasibility.
 * @param boundLower Lower bounds for variables.
 * @param boundUpper  Upper bounds for variables.
 * @param lhs LHS for inequalities.
 */
void logEntranceToCheckIfFeasible(
        const DualVariables& variables,
        const std::vector<double>& boundLower,
        const std::vector<double>& boundUpper,
        const std::vector<double>& lhs);
/**
 * @brief Logs lower bound failure in checkIfFeasible.
 * 
 * @param difference The error value.
 */
void logLowerBoundFailureInCheckIfFeasible(double difference);
/**
 * @brief Logs upper bound failure in checkIfFeasible.
 * 
 * @param difference The error value.
 */
void logUpperBoundFailureInCheckIfFeasible(double difference);
/**
 * @brief Logs LHS inequality violation in checkIfFeasible.
 * 
 * @param error The error value.
 */
void logLhsInequalityFailureInCheckIfFeasible(double error);
/**
 * @brief Logs that checkIfFeasible is finished.
 * 
 * @param value The converted-to-string boolean response of checkIfFeasible.
 */
void logExitFromCheckIfFeasible(const std::string& value);

/**
 * @brief Logs subgradient.
 * 
 * @param current The subgradient value.
 */
void logSubgradient(const SubgradientOptimizationParameters& current);

/**
 * @brief Logs hitMu and hitLambda.
 * 
 * @param hitMu Vector with hitMu.
 * @param hitLambda Vector with hitLambda.
 */
void logHitMuAndLambda(const vector<int>& hitMu, const vector<int>& hitLambda);

/**
 * @brief Logs indices where the itinerary planes were hit.
 * 
 * @param hitIdx The vector with indices of those itineraries that are hit.
 */
void logPlaneIntersectionItineraries(const vector<int>& hitIdx);

/**
 * @brief Logs that cutting plane optimization is started.
 * 
 * @param state The current trajectory evaluation state.
 * @param dualPtr Pointer to starting dual variables.
 */
void logStartCuttingPlaneOptimization(const State& state, DualVariables* dualPtr);
/**
 * @brief Logs dual variables in cutting plane iteration.
 * 
 * @param iter The iteration index.
 * @param dualVariables The variables to log.
 * @param feasible If the variables are feasible.
 */
void logCuttingPlaneIterationDuals(
        unsigned iter, const DualVariables& dualVariables, bool feasible);
/**
 * @brief Logs dualHistory size.
 * 
 * @param historySize The size of dualHistory.
 */
void logDualHistorySize(std::size_t historySize);

/**
 * @brief Logs objective and dual variable statistics.
 * 
 * @param cuttingPlaneObjective The cutting plane current objective.
 * @param regObjectiveValue The regularized cppad objective value.
 * @param dualVariables Current dual variables.
 * @param prevDuals Previous dual variables.
 */
void logObjectiveAndDualNorms(double cuttingPlaneObjective, double regObjectiveValue,
        const DualVariables& dualVariables, const DualVariables& prevDuals);
/**
 * @brief Logs that cutting plane iteration is finished.
 * 
 * @param iter The iteration index.
 * @param absObjectiveDiff Absolute objective difference.
 * @param absShiftDiff Absolute value of the difference between value at point and 
 *      simplex-estimated value.
 * @param relShiftDiff Relative error between function value at point and simplex-estimated value.
 */
void logFinishedCuttingPlaneIteration(unsigned iter, double absObjectiveDiff,
        double absShiftDiff, double relShiftDiff);
/**
 * @brief Logs that simplex is restarted from deque.
 */
void logRestartSimplexFromDeque();
/**
 * @brief Logs that this iteration is final cutting plane iteration.
 * 
 * @param iterId The iteration index.
 */
void logLastCuttingPlaneIteration(unsigned iterId);
/**
 * @brief Logs how cutting plane dual variable point is changed.
 * 
 * @param lowerChange Lower bound change. First entry is for lambda, second is for mu.
 * @param upperChange Upper bound change. First entry is for lambda, second is for mu.
 * @param upperHitCount Upper bound hit count. First entry is for lambda, second is for mu.
 * @param lowerHitCount Lower bound hit count. First entry is for lambda, second is for mu.
 * @param targetMuCount Number of mu variables.
 * @param targetLambdaCount Number of lambda variables.
 * @param prevPlaneHits Previous plane hits.
 * @param currentPlaneHits Current plane hits.
 */
void logChangeInCuttingPlane(
        const std::pair<size_t, size_t>& lowerChange,
        const std::pair<size_t, size_t>& upperChange,
        const std::pair<size_t, size_t>& upperHitCount,
        const std::pair<size_t, size_t>& lowerHitCount,
        size_t targetMuCount,
        size_t targetLambdaCount,
        const vector<bool>& prevPlaneHits,
        const vector<bool>& currentPlaneHits);

/**
 * @brief Log processing unchecked and immortal in dual deque.
 */
void logProcessingImmortalUnchecked();
/**
 * @brief Log norms for immortal dual deque entry. 
 * 
 * @param info Dual deque entry.
 */
void logImmortalNorms(const DualDequeInfo& info);
/**
 * @brief Log that immortal dual deque entry is processed.
 * 
 * @param writeLogOut If true, output is written. Not written otherwise.
 * @param info Dual deque entry.
 */
void logImmortalProcessed(bool writeLogOut, const DualDequeInfo& info);

} // namespace backend
} // namespace sea

