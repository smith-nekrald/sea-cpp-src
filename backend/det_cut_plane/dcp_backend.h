/**
 * @file dcp_backend.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares Deterministic Cutting Plane backend for allotment decision and related
 * corresponding API.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../ipopt/ipopt_backend.h"
#include "../../manager.h"
#include "index_map.h"
#include "cbc_pre_map.h"

#include <cppad/ipopt/solve.hpp>

namespace sea {
namespace backend {

/**
 * @brief Configures deterministic cutting plane backend.
 */
struct DetCutPlaneBackendConfig {
    /// @brief Manager with InputData.
    ConstInputManagerPtr inputManager;
    /// @brief Manager with InputLinks.
    ConstLinksManagerPtr linksManager;

    // These values must be equal!
    /// @brief Command line log-level for Cbc.
    unsigned cbcLogLevel = 3;
    /// @brief File log-level for Cbc.
    unsigned cbcFileLogLevel = 3;

    /// @brief Seed for repeatable randomness.
    unsigned seed = 124124;
    /// @brief Maximal number of iterations.
    unsigned iterations = 100;
    /// @brief Initial amount of generated planes.
    unsigned initialPlanes = 30;

    /// @brief Desired precision.
    double needError = 0.01;

    /// @brief Whether to stop when solution becomes infeasible.
    bool stopOnInfeasible = false;

    /// @brief Tolerance to integral solutions.
    double integerTolerance = 1e-2;

    /// @brief Whether algorithm may tolerate constraints.
    bool tolerateConstraints = true;

    /// @brief If true, efficient manual RAM memory management is applied.
    bool needMemory = false;

    /// @brief Default capacity utilization ratio. Sometimes incomplete capacity utilization
    /// allows to tweak for better performance.
    double defaultUtilizationRatio = 1.0;
};

/**
 * @brief Backend for making allotment decision with Deterministic Cutting Plane method.
 */
class DetCutPlaneBackend {
public:
    /**
     * @brief Constructor for Deterministic Cutting Plane Backend.
     * 
     * @param config Configuration for Deterministic Cutting Plane backend.
     */
    DetCutPlaneBackend(const DetCutPlaneBackendConfig& config);
    /**
     * @brief Suggests allotments and fills corresponding Decision Manager.
     * 
     * @param objectiveValue When requested (i.e. not nullptr), objective value is written there.
     * @return Decision Manager with allotment.
     */
    DecisionManagerPtr provideAllotments(double* objectiveValue = nullptr);
    /**
     * @brief Set the capacity utilization ratio.
     * 
     * @param value Value of capacity utilization. Should arrive from segment [0,1].
     */
    void setUtilizationRatio(double value) {
        utilizationRatio = value;
    }
    /**
     * @brief Virtual destructor for efficient C++ polymorphism.
     */
    virtual ~DetCutPlaneBackend() {}

private:
    /**
     * @brief Initializes constraint bounds.
     * 
     * @param glowerPtr  Pointer to lower constraint bounds.
     * @param gupperPtr  Pointer to upper constraint bounds.
     */
    void initConstraintsLR(vector<double>* glowerPtr, vector<double>* gupperPtr);
    /**
     * @brief Initializes variable bounds.
     * 
     * @param vlowerPtr Pointer to lower variable bounds.
     * @param vupperPtr Pointer to upper variable bounds.
     */
    void initBoundsLR(vector<double>* vlowerPtr, vector<double>* vupperPtr);
    /**
     * @brief Setups the main problems.
     */
    void setupMainProblem();
    /**
     * @brief Runs Cbc optimization.
     * 
     * @return Objective value.
     */
    double runCbc();
    /**
     * @brief Adds constraints to Cbc.
     */
    void addConstraints();
    /**
     * @brief Generates random solution (e.g. for initialization).
     */
    void genRandomSolution();
    /**
     * @brief Calculates error.
     * 
     * @return Current error between Cbc objective and function at point.
     */
    double calcError();
    /**
     * @brief Fills decision with currently optimal allotment.
     * 
     * @param decision The decision to fill.
     */
    void fillDecision(Decision* decision);
    /**
     * @brief Checks feasibility for constraints and bounds.
     * 
     * @return double Maximal absolute constraint violation.
     */
    double checkConstraintsAndBounds();

private:
    /**
     * @brief Logs iteration in provide allotments function.
     * 
     * @param iterCount The iteration index.
     * @param error Current objective error.
     * @param constraintError Current constraint error.
     * @param accumulatedConstraintError Current total constraint error.
     * @param objectiveEstimation Estimated objective value.
     */
    void logIterationInProvideAllotments(
            unsigned iterCount, double error, double constraintError,
            double accumulatedConstraintError, double objectiveEstimation) const;

private:
    /// @brief Data structure for efficiently building Cbc problem.
    CbcPreMap cbcLastProblem;
    /// @brief Deterministic Cutting Plane configuration.
    DetCutPlaneBackendConfig config;
    /// @brief Manager with DCP index.
    DcpIndexManagerPtr indexManager;
    /// @brief Last solution in double format.
    vector<double> lastSolution;
    /// @brief Objective at the last solution.
    double preparedSolutionObjective;
    /// @brief Maximal capacity utilization ratio.
    double utilizationRatio;
};

/**
 * @brief Generic method to add multi-variable constraint.
 * 
 * @param[in] variables Information about which variable indices to multiply with coefficient.
 * @param[in] coefficient The coefficient for additional multiplication. I.e. multiplication is
 * both over coefficient and entry.coef.
 * @param[in] constraintId Target constraint index.
 * @param[out] constraints Vector with all constraints.
 */
void addMultiVarConstraint(const vector<CoefIndex>& variables,
                           double coefficient, unsigned constraintId,
                           vector<vector<CoefIndex>>& constraints);
/**
 * @brief Generic method to add multi-variable constraint.
 * 
 * @param[in] variables Information about which variable indices to multiply with coefficient.
 * @param[in] coefficient The coefficient for multiplication.
 * @param[in] constraintId Target constraint index.
 * @param[out] constraints Vector with all constraints.
 */
void addMultiVarConstraint(const vector<unsigned>& variables,
                           double coefficient, unsigned constraintId,
                           vector<vector<CoefIndex>>& constraints);
/**
 * @brief Generic method to set coefficient for multiple variables in objective.
 * 
 * @param[in] variables The variables for providing indices and coefficients.
 * @param[in] coefficient Additional coefficient for multiplication.
 * @param[out] objective Target objective to modify.
 */
void addMultiVarObjective(
        const vector<CoefIndex>& variables, double coefficient, vector<double>& objective);
/**
 * @brief Generic method to set coefficient for multiple variables in objective.
 * 
 * @param[in] variables The variables for providing indices for multiplication.
 * @param[in] coefficient Coefficient to multiply with.
 * @param[out] objective The objective to modify.
 */
void addMultiVarObjective(
        const vector<unsigned>& variables, double coefficient, vector<double>& objective);
/**
 * @brief Calculate revenue. 
 * 
 * @param demand The object with demand entity.
 * @param demandValue The value of demand.
 * @return Resulting revenue r(d) = d * p(d).
 */
double calculateRevenue(const Demand& demand, double demandValue);
/**
 * @brief Calculate revenue derivative.
 * 
 * @param demand The object with demand entity.
 * @param demandValue The value of demand.
 * @return Revenue derivative in point.
 */
double calculateRevenueDerivative(const Demand& demand, double demandValue);

} // namespace backend
} // namespace sea
