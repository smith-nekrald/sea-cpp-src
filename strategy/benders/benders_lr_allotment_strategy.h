/**
 * @file benders_lr_allotment_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares Benders Allotment Strategy, related structures and corresponding API.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../abstract_allotment_strategy.h"
#include "../../backend/benders/benders_allotment_backend.h"
#include "../../backend/lagrangian_relaxation/lagrangian_relaxation_backend.h"
#include "../../backend/ipopt/ipopt_backend.h"

#include <limits>

namespace sea {
namespace strategy {

/// @brief Maximal possible double value.
const double DOUBLE_INF = std::numeric_limits<double>::max();

/**
 * @brief Enumerate the type of initialization.
 */
enum class InitializationType {
    IPOPT_ALLOTMENTS,   ///< Get allotments from Ipopt-optimized fluid approximation.
    DETERMINISTIC_CUTTING_PLANE_ALLOTMENTS, ///< Get allotments from DetCutPlane approach.
    ZERO_ALLOTMENTS     ///< Initial allotment point - no allotments at all.
};

/**
 * @brief Configuration for Benders algorithm organization.
 */
struct StrategyBendersInnerConfig {
    /// @brief Coefficient to blend Ipopt-objective and LR-objective.
    double objectiveBlending = 1.0;
    /// @brief Stop criterion.
    ConvergenceCriterionType stopStrategy = ConvergenceCriterionType::BOTH;
    /// @brief Maximal number of iterations.
    std::size_t iterationCount = 30;
    /// @brief Approach for getting starting point for Benders iterations.
    InitializationType initType = InitializationType::IPOPT_ALLOTMENTS;
    /// @brief Desired precision.
    double bendersStopPrecision = 1e-5;
};

/**
 * @brief Configuration for Benders Allotment Strategy.
 */
struct BendersLRAllotmentStrategyConfig {
    /// @brief Holder wtih backends.
    BackendConfigHolder backendConfigs;
    /// @brief Generic config for allotment strategy.
    AllotmentStrategyConfig abstractConfig;
    /// @brief Configuration for algorithm organization.
    StrategyBendersInnerConfig config;
};

/**
 * @brief Entity to control for objective value and allotment selection point.
 */
struct VariableGroup {
    /// @brief The value of Ipopt-defined fluid approximation objective.
    double ipoptObjective = -DOUBLE_INF;
    /// @brief The value of LR-based objective.
    double lrObjective = -DOUBLE_INF;
    /// @brief Blended between ipoptObjective and lrObjective.
    double blendedObjective = -DOUBLE_INF;
    /// @brief The allotment selection point.
    vector<bool> allotmentSelection;
};

/**
 * @brief Comparison operator between variable groups. Helps to sort.
 * Comparison is by blendedObjective.
 * 
 * @param lhs Left-hand-side entry.
 * @param rhs Right-hand-side entry.
 * @return True if LHS < RHS, and false otherwise.
 */
bool operator<(const VariableGroup& lhs, const VariableGroup& rhs);

/**
 * @brief Blends objectives with coefficient weight from [0, 1]
 * 
 * @param deterministicObjective The objective obtained by fluid approximation.
 * @param methodObjective The objective computed with Lagrangian Relaxation.
 * @param weight Blending coefficient from [0, 1].
 * @return The blended objective value.
 */
double objectiveBlending(
        double deterministicObjective, double methodObjective, double weight = 1.0);

/**
 * @brief Allotment strategy based on Benders Decomposition. Essentially, interface
 * for calling Benders Decomposition backend.
 */
class BendersLRAllotmentStrategy : public AbstractAllotmentStrategy {
public:
    /**
     * @brief Constructor for Benders Allotment Strategy.
     * 
     * @param aConfig Configuration for Benders Allotment Strategy.
     */
    BendersLRAllotmentStrategy(const BendersLRAllotmentStrategyConfig& aConfig);
    /**
     * @brief Resets algorithm state without resetting backends.
     */
    virtual void reset() override;
    /**
     * @brief Method to make allotment decision. Writes the made decision into the correct
     * fields of Decision Manager.
     * 
     * @return Pointer to Decision Manager with allotment decision.
     */
    virtual DecisionManagerPtr provideAllotments() override;
    /**
     * @brief Virtual destructor for efficient C++ polymorphism.
     */
    virtual ~BendersLRAllotmentStrategy() {};

private:
    /**
     * @brief Logs iteration in Benders Allotment Strategy.
     * 
     * @param iterId Index of the iteration.
     * @param lrObjective Objective based on Lagrangian Relaxation.
     * @param ipoptObjective Objective based on Fluid Approximation.
     * @param blendedObjective Blended objective.
     */
    void logIteration(std::size_t iterId, double lrObjective,
            double ipoptObjective, double blendedObjective) const;
    /**
     * @brief Logs variable group update.
     * 
     * @param group The variable group to log.
     */
    void logUpdateGroup(const VariableGroup& group) const;
    /**
     * @brief Logs that iterations are stopped.
     */
    void logStopIterations() const;

private:
    /// @brief Benders allotment strategy configuration.
    const StrategyBendersInnerConfig innerConfig;
};

} // namespace strategy
} // namespace sea

