/**
 * @file optimization_problem.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements OptimizationProblem, which is a way to describe the optimization objective
 * and constraints in a CppAD-friendly format.
 *
 * @copyright Smith School of Business (c) 2023
 */
#pragma once

#include "../../manager.h"
#include "index_map.h"
#include <cppad/ipopt/solve.hpp>

namespace sea {
namespace backend {

/**
 * @brief Configures Optimization Problem.
 */
struct OptimizationConfig {
    /// @brief Manager for Ipopt Index, helps to convert regular indexation into the
    /// indexation for optimization problem and backwards.
    ConstIpoptIndexManagerPtr indexManager;
    /// @brief Manager for InputData. Dataset with static information.
    ConstInputManagerPtr inputManager;
    /// @brief Manager for InputLinks. Dataset with data structures over static information.
    ConstLinksManagerPtr linksManager;

    /// @brief Manager for the current Decision.
    ConstDecisionManagerPtr decisionManager;
    /// @brief Manager for the current Action.
    ConstActionManagerPtr actionManager;

    /// @brief The time of the last processed event. Allows considering previous decisions
    /// as already made with known fixed values (derived from Decision and Action).
    unsigned updateTime = 0;

    /// @brief Capacity utilization ratio. Sometimes restricting for filling capacity
    /// completely helps policy to get more effective.
    double utilizationRatio = 1.0;
};

/**
 * @brief OptimizationProblem is a way to describe the optimization objective and constraints
 * in a CppAD-friendly format. CppAD calls operator() with two arguments: the functions to form,
 * and the variables to use. The OptimizationProblem is an object-oriented implementation for
 * such callable.
 */
class OptimizationProblem {
public:
    /// @brief CppAD-style notation.
    typedef vector<CppAD::AD<double>> ADvector;

public:
    /**
     * @brief Constructs a new Optimization Problem.
     *
     * @param aConfig Optimization problem configuration.
     */
    OptimizationProblem(const OptimizationConfig& aConfig);
    /**
     * @brief The operator called by CppAD.
     *
     * @param functions The functions to form. Where functions[0] is objective, the rest are
     * constraints.
     * @param variables The variables to use for forming objectives and constraints.
     */
    void operator()(ADvector& functions, const ADvector& variables);

protected:
    /**
     * @brief Processes one pricing event. Updates corresponding objective and constraints.
     * Helps when going through the events inside the operator() called from CppAD.
     *
     * @param[in] timeNow The time of considered pricing event.
     * @param[in] event  A pricing event to process.
     * @param[in] variables  The vector with variables.
     * @param[out] functionsPtr A pointer to a vector with functions.
     * @param[out] bookingsPtr A pointer to a vector for tracking bookings.
     */
    void processPricing(const unsigned timeNow,
                   const InputData::Event& event,
                   const vector<CppAD::AD<double>>& variables,
                   vector<CppAD::AD<double>>* functionsPtr,
                   vector<CppAD::AD<double>>* bookingsPtr) const;
    /**
     * @brief Processes one cutoff event. Updates corresponding objective and constraints.
     * Helps when going throught the events inside the operator() called from CppAD.
     *
     * @param[in] timeNow The time of considered cutoff event.
     * @param[in] event  The cutoff event to process.
     * @param[in] variables The vector with variables.
     * @param[out] functionsPtr A poiner to a vector with functions.
     * @param[out] bookingsPtr A pointer to a vector for tracking bookings.
     * @param[out] shippedPtr A pointer to a vector for tracking the shipped containers.
     * @param[out] containersPtr A pointer to a vector for tracking containers in ports.
     */
    void processCutoff(const unsigned timeNow,
                   const InputData::Event& event,
                   const vector<CppAD::AD<double>>& variables,
                   vector<CppAD::AD<double>>* functionsPtr,
                   vector<CppAD::AD<double>>* bookingsPtr,
                   vector<CppAD::AD<double>>* shippedPtr,
                   vector<CppAD::AD<double>>* containersPtr) const;
    /**
     * @brief  Processes one arrival event. Updates corresponding objective and constraints.
     * Helps when going through the events inside the operator() called from CppAD.
     *
     * @param[in] event The arrival event to process.
     * @param[in] variables A vector with variables.
     * @param[out] functionsPtr A poiner to a vector with functions.
     * @param[out] shippedPtr A pointer to a vector for tracking the shipped containers.
     * @param[out] containersPtr A pointer to a vector for tracking containers in ports.
     */
    void processArrival(const InputData::Event& event,
                   const vector<CppAD::AD<double>>& variables,
                   vector<CppAD::AD<double>>* functionsPtr,
                   vector<CppAD::AD<double>>* shippedPtr,
                   vector<CppAD::AD<double>>* containersPtr) const;

    /**
     * @brief Forms capacity constraints.
     *
     * @param[in] shipped Vector tracking amount of shipped containers.
     * @param[in] functionsPtr A pointer to a vector with functions.
     */
    void formCapacityConstraints(const vector<CppAD::AD<double>>& shipped,
            vector<CppAD::AD<double>>* functionsPtr) const;
    /**
     * @brief Forms constraints about the final amount of containers in ports.
     * Updates objective, if relevant.
     *
     * @param containers Vector tracking amount of containers in ports.
     * @param functionsPtr A pointer to a vector with functions.
     */
    void formFinalContainerConstraints(const vector<CppAD::AD<double>>& containers,
            vector<CppAD::AD<double>>* functionsPtr) const;
    /**
     * @brief Forms one-group constraints. At most one allotment from each group can get selected.
     *
     * @param variables Vector with original variables.
     * @param functionsPtr A pointer to a vector with functions.
     */
    void formGroupConstraints(const vector<CppAD::AD<double>>& variables,
                   vector<CppAD::AD<double>>* functionsPtr) const;
    /**
     * @brief Releases managers. Helps for granular-level memory management.
     */
    void releaseManagers();

private:
    /// @brief Configuration for optimization problem. Contains static
    // structures, the current Action and Decision, etc.
    const OptimizationConfig config;
};

} // namespace backend
} // namespace sea
