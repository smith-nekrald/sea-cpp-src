/**
 * @file ipopt_backend.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Describes IpoptBackend entity and corresponding API. 
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../../manager.h"
#include "index_map.h"
#include "../../algorithm/state.h"
#include "../dual_variables.h"

#include <cppad/ipopt/solve.hpp>

namespace sea {
namespace backend {

/**
 * @brief Describes memory management and/or approximations for Ipopt when computing Hessian.
 */
enum class MemoryStrategy {
    EXACT_COMPUTATION,      ///< Computing Hessian completely.
    LIMITED_MEMORY_BFGS,    ///< BFGS approximation when computing Hessian.
    LIMITED_MEMORY_SR1,     ///< SR1 approximation when computing Hessian.
};

/**
 * @brief Describes CppAD Auto-Derivation strategy.
 */
enum class DerivativeStrategy {
    FORWARD,    ///< Forward Auto-Derivatives.
    BACKWARD    ///< Backward Auto-Derivatives.
};

/**
 * @brief Describes type of Linear Solver to used in Ipopt Solver.
 */
enum class LinearSolver {
    MA27,   ///< HSL-MA27 Linear Solver.
    MA57,   ///< HSL-MA57 Linear Solver.
    MA86,   ///< HSL-MA86 Linear Solver.
    MA97,   ///< HSL-MA97 Linear Solver.
    MA77,   ///< HSL-MA77 Linear Solver.
    MUMPS,  ///< MUMPS Linear Solver.
};

/**
 * @brief Configures Ipopt Backend.
 */
struct IpoptBackendConfig {
    /// @brief Manager for Input Data.
    ConstInputManagerPtr inputManager;
    /// @brief  Manager for Input Links.
    ConstLinksManagerPtr linksManager;

    /// @brief Type of memory management when computing Hessian.
    MemoryStrategy memoryManagement = MemoryStrategy::EXACT_COMPUTATION;
    /// @brief Strategy for CppAD auto-derivation.
    DerivativeStrategy derivativeComputation = DerivativeStrategy::BACKWARD;
    /// @brief Type of linear solver to use in Ipopt.
    LinearSolver solver = LinearSolver::MA27;

    /// @brief Level of verbose output while running Ipopt solver to log file.
    int filePrintLevel = 5;
    /// @brief Level of verbose output while running Ipopt solver to CLI.
    int printLevel = 5;

    /// @brief If true, backend tends to optimize RAM memory performance.
    bool needMemory = false;
    /// @brief If true, index contains human-readable descriptions. Helps for debugging.
    bool needDescriptionsInIndex = false;

    /// @brief Path prefix for saving Ipopt solver logs.
    std::string path_prefix = "";

    /// @brief If true, saves last feasible point, which allows to warm-start later. 
    bool saveVariables = true;

    /// @brief Sometimes non-complete capacity utilization helps with performance tuning.
    double defaultUtilizationRatio = 1.0;
};

/**
 * @brief Entity for performing all operations for fluid approximation when optimized with Ipopt.
 */
class IpoptBackend {
public:
    /**
     * @brief Constructs a new IpoptBackend object.
     * 
     * @param config Backend Configuration.
     */
    IpoptBackend(const IpoptBackendConfig& config);
    /**
     * @brief Updates inner state and variables assuming that the decision is valid for a prefix
     * until specified time in timeParameters.
     * 
     * @param[in] timeParameters Specifies the state of interaction. Tracks event time and the
     * stage of interaction protocol.
     * @param[out] decision The Manager with Decision entity. Place to write the decisions,
     * and place to look for decisions that are already made.
     * @param[in] action The Manager with Action entity. Allows to recover response of environment.
     * @param[out] objectiveEstimation Place to write estimated objective. Estimation is written
     * if the pointer is not nullptr. Objective estimation is how Ipopt solver computed objective.
     * @param[out] duals Place to write dual variables. Duals are written if the pointer is not 
     * nullptr. Helps to create starting point for methods based on Lagrangian Relaxation.
     */
    void moveDecisionToTime(
            TimeParameters timeParameters,
            DecisionManagerPtr decision,
            ConstActionManagerPtr action,
            double* objectiveEstimation = nullptr,
            DualVariables* duals = nullptr);
    /**
     * @brief Method to call from Benders-Decomposition Allotment approach.
     * 
     * @param[in] allotments The allotments selected in Benders Decomposition approach.
     * @param[out] objectiveEstimation Pointer to a place for writing objective estimation.
     * @param[out] duals Pointer to a place for writing dual variables.
     * @return Smart pointer to a Manager with Decision.
     */
    DecisionManagerPtr bendersQuery(
            const vector<bool>& allotments,
            double* objectiveEstimation = nullptr,
            DualVariables* duals = nullptr);
    /**
     * @brief Requesting for allotment decision. 
     * 
     * @param[out] objectiveEstimation Place to write objective estimation, in case the pointer is 
     * distinct from nullptr.
     * @return Vector with selected allotments. 
     */
    vector<bool> provideAllotments(double* objectiveEstimation = nullptr);
    /**
     * @brief Sets value for ignoreSpot parameter. Depending on this parameter value,
     * the spot market is either ignored or not.
     * 
     * @param value Value to set.
     */
    void setIgnoreSpot(bool value) {
        ignoreSpot = value;
    }
    /**
     * @brief Sets the capacity utilization ratio. Sometimes having this ratio less than 1.0
     * is helpful for performance tuning.
     * 
     * @param value The new value for capacity utilization ratio.
     */
    void setUtilizationRatio(double value) {
        utilizationRatio = value;
    }

protected:
    /**
     * @brief Computes the lower and upper bounds for constraint expressions.
     *
     * @param[out] glowerPtr Pointer to a vector that gets lower bounds filled.
     * @param[out] gupperPtr Pointer to a vector that gets upper bounds filled.
     */
    void initConstraintsLR(vector<double>* glowerPtr, vector<double>* gupperPtr) const;

    /**
     * @brief Computes the lower and upper bounds for optimization variables.
     *
     * @param[out] vlowerPtr Pointer to a vector that gets lower bounds filled.
     * @param[out] vupperPtr Pointer to a vector that gets upper bounds filled.
     */
    void initBoundsLR(vector<double>* vlowerPtr, vector<double>* vupperPtr) const;

    /**
     * @brief Method to solve optimization problem, or to update current solution.
     * Multiple scenarios are possible, depending on the requested input.
     *
     * @param[in] timeParameters Structure to indicate the state of evaluation and relative
     * time of events.
     * @param[out] decisionManagerToWrite  Manager with Decision entity. Gets updated when
     * the corresponding flags configure that.
     * @param[in] currentActionManager Manager with Action entity. Allows to specify trajectory
     * prefix.
     * @param[in] writeToDecision Flag to indicate whether the optimization results needs
     * to get written in the decision. Sometimes only duals are needed, and decision is not
     * supposed to get modified.
     * @param[out] allotmentsToSelect If not nullptr, fills with the selected allotments.
     * @param[out] objectiveEstimation If not nullptr, fills with the Ipopt optimization
     * problem value.
     * @param[out] duals If not nullptr, fills the duals structure with corresponding dual
     * variables in the optimization program.
     */
    void recalculate(TimeParameters timeParameters,
            DecisionManagerPtr decisionManagerToWrite,
            ConstActionManagerPtr currentActionManager,
            bool writeToDecision = true,
            vector<bool>* allotmentsToSelect = nullptr,
            double* objectiveEstimation = nullptr,
            DualVariables* duals = nullptr);
private:
    /**
     * @brief Initializes the corresponding to allotments optimization variables and their bounds
     * according to the values available in the decision.
     * Assumes that allotments are already decided and are available in the decision.
     *
     * @param[in] decisionManager The manager storing the Decision entity.
     * @param[out] vlowerPtr Lower bounds on the variables in the optimization problem.
     * @param[out] vupperPtr Upper bounds on the variables in the optimization problem.
     * @param[out] variablesPtr Initial values for the variables in the optimization problem.
     */
    void initializeSuppliedAllotments(
            ConstDecisionManagerPtr decisionManager,
            vector<double>* vlowerPtr,
            vector<double>* vupperPtr,
            vector<double>* variablesPtr) const;

    /**
     * @brief Supplies informatino available through decisions and actions in the trajectory prefix.
     *
     * @param[in] timeParameters Structure with information about considered event relative time
     * and evaluation state.
     * @param[in] decisionManager Decision manager with prefix decision information.
     * @param[in] currentActionManager Action manager storing prefix actions.
     * @param[out] vlowerPtr Lower bounds on the variables in the optimization problem.
     * @param[out] vupperPtr Upper bounds on the variables in the optimization problem.
     * @param[out] variablesPtr  Initial values for the variables in the optimization problem.
     */
    void setPreviouslyMadeDecisions(
            const TimeParameters& timeParameters,
            ConstDecisionManagerPtr decisionManager,
            ConstActionManagerPtr currentActionManager,
            vector<double>* vlowerPtr,
            vector<double>* vupperPtr,
            vector<double>* variablesPtr) const;

    /**
     * @brief Method to write the selected allotments to Decision. May update Decision Manager,
     * if the corresponding flag requests that. May also write the result to the vector
     * allotmentsToSelect, if the pointer is not nullptr.
     *
     * @param[in] solutionValues Vector with solution values after the optimization problem
     * is solved.
     * @param[out] decisionManagerToWrite Stores the decision, which is updated with new
     * allotments s.t. the flag writeToDecision.
     * @param[in] writeToDecision  If true, the decision in Decision manager is updated s.t. the
     * selected allotments. If false, not updated.
     * @param[out] allotmentsToSelect  If not null, filled s.t. the vector
     * stores the information on selected allotments.
     * @param[out] vlowerPtr Lower bounds on variables in the optimization problem.  Modified subject
     * to the decided allotments.
     * @param[out] vupperPtr Upper bounds on variables in the optimization problem. Modified subject
     * to the decided allotments.
     * @param[out] variablesPtr Feasible point in the optimization problem. Modified subject to
     * the decided allotments.
     */
    void supplyAllotmentsFromSolution(
            const vector<double>& solutionValues,
            DecisionManagerPtr decisionManagerToWrite,
            bool writeToDecision,
            vector<bool>* allotmentsToSelect,
            vector<double>* vlowerPtr,
            vector<double>* vupperPtr,
            vector<double>* variablesPtr) const;

    /**
     * @brief Retrieves dual variables from Ipopt solution.
     *
     * @param[in] lambdas The dual variables obtained while solving Ipopt problem.
     * @param[out] duals  The structure to output the dual variables. Writing only happens if
     * duals != nullptr.
     */
    void formDuals(const vector<double>& lambdas, DualVariables* duals) const;

    /**
     * @brief Writes the result of optimization into a Decision (except allotments, there is
     * a specific method for this purpose).
     *
     * @param[in] timeParameters The structure with information about the relative times of events
     * and evaluation status.
     * @param[in] solutionValues Solution provided by Ipopt optimization.
     * @param[out] decisionManagerToWrite Manager with a Decision entity to fill.
     */
    void writeSolutionToDecision(
            const TimeParameters& timeParameters,
            const vector<double>& solutionValues,
            DecisionManagerPtr decisionManagerToWrite) const;

    /**
     * @brief Initializes variables somewhere between bounds (vlower and vupper).
     * If canInitVariables is true and lastVariables are available, initializes with
     * previous optimal point.
     *
     * @param[in] vlower Variable lower bounds.
     * @param[in] vupper Variable upper bounds.
     * @param[out] variablesPtr A point to initialize. The result is assumed feasible.
     */
    void initVariables(
            const vector<double>& vlower,
            const vector<double>& vupper,
            vector<double>* variablesPtr) const;


protected:
    /// @brief Backend configuration. Provides static data structures and specifies
    /// optimization parameters and behaviors.
    IpoptBackendConfig config;

    /// @brief Manager for the Ipopt index. The index is responsible for converting
    /// from internal Ipopt indexation format to the format to use for filling Decision.
    IpoptIndexManagerPtr indexManager;

    /// @brief Indicates if there is an option to initialize
    /// variables from the previous optimal point.
    bool canInitVariables;

    /// @brief If true, spot market is ignored. Allows to evaluate allotment-only strategy.
    bool ignoreSpot;

    /// @brief Previous optimal point. Useful for initialization.
    vector<double> lastVariables;

    /// @brief Capacity utilization ratio. Sometimes helps for performance tuning.
    double utilizationRatio;
};

/**
 * @brief Method to update lower bound. Assign value to a maximum from value and new estimation.
 * 
 * @tparam T Type of the values to process.
 * @param value The value to update, previous lower bound.
 * @param candidate The new lower bound candidate.
 */
template<typename T>
void updateLower(T& value,  T candidate) {
    value = std::max(value, candidate);
}

/**
 * @brief Method to update upper bound. Assigns value to a minimum from value and new estimation.
 * 
 * @tparam T Type of the values to process.
 * @param value The value to update, previous upper bound.
 * @param candidate The new upper bound candidate.
 */
template<typename T>
void updateUpper(T& value, T candidate) {
    value = std::min(value, candidate);
}

/**
 * @brief Logs Ipopt options into debug-level sublogger (Ipopt Backend).
 * 
 * @param options Ipopt Solver options.
 */
void logOptions(const std::string& options);

/**
 * @brief Makes Ipopt options string from Ipopt configuration.
 * 
 * @param config The Ipopt backend configuration.
 * @return std::string The string with Ipopt configuration.
 */
std::string makeOptionsFromConfig(const IpoptBackendConfig& config);

/**
 * @brief Logs constraints to the logger on debug-level output. The most 
 * likely use-case is for IPOPT backend, but if the index map is in the 
 * same format, the other backend-targeted output is supported too.
 * 
 * @param vlower Lower variable bounds.
 * @param vupper Upper variable bounds. 
 * @param glower  Lower constraint bounds.
 * @param gupper  Upper constraint bounds.
 * @param variables The array with variable values (feasible point). 
 * @param index Ipopt variable index.
 * @param backendType Type of backend to attribute the log to. The pre-set option is Ipopt backend.
 */
void logConstraints(
        const vector<double>& vlower, 
        const vector<double>& vupper,
        const vector<double>& glower, 
        const vector<double>& gupper,
        const vector<double>& variables, 
        const IpoptIndexMap& index,
        BackendType backendType = BackendType::IPOPT);

/**
 * @brief Logs the allotments into INFO logging stream.
 *
 * @param acceptedAllotments The allotments to log.
 * @param backendType The type of backend (for selecting the corresponding log output).
 */
void logSelectedAllotments(const vector<bool>& acceptedAllotments, BackendType backendType);

} // namespace backend
} // namespace sea
