/**
 * @file lagrangian_relaxation_backend.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements Lagrangian Relaxation backend. This entity is mostly responsible for
 * building shadow price of capacity (DualVariables) and making decisions based on duals.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "gm_sea/gm_src/gm_types.h"
#include "../../manager.h"
#include "../../common.h"
#include "../../algorithm/state.h"
#include "index.h"
#include "structures.h"

#include <vector>
#include <unordered_map>
#include <random>
#include <deque>
#include <experimental/optional>

#include <CoinBuild.hpp>
#include <OsiClpSolverInterface.hpp>
#include <CoinHelperFunctions.hpp>


namespace sea {
namespace backend {

using std::size_t;
using std::experimental::optional;

/**
 * @brief Enumerates the way to compute CLP solution.
 */
enum class ClpSolutionConfig {
    CLP_PRIMAL, ///< Primal Simplex.
    CLP_DUAL    ///< Dual Simplex.
};

/**
 * @brief Enumerates cutoff decision type.
 */
enum class CutoffDecisionType {
    BY_ITINERARY,  ///< Itinerary-based decision at cut-off.
    BY_EVENT       ///< Event-based decision at cut-off.
};

/**
 * @brief Enumerates random initialization.
 */
enum class RandomInitType {
    RANDOM_NORMAL,  ///< With normal distribution.
    RANDOM_UNIFORM, ///< With uniform distribution.
    RANDOM_BOTH     ///< Both uniform and normal distribution.
};

/**
 * @brief Enumerate optimization approach.
 */
enum class OptimizationAlgo {
    CUTTING_PLANE, ///< Optimization with cutting plane.
    GM             ///< Optimization by gradient method.
};

/**
 * @brief Configures Lagrangian Relaxation Backend.
 */
struct LagrangianRelaxationBackendConfig {
    // Common entries related to problem structure.

    /// @brief Manager with InputData.
    ConstInputManagerPtr inputManager = nullptr;
    /// @brief Manager with InputLinks.
    ConstLinksManagerPtr linksManager = nullptr;
    /// @brief Whether fancy memory optimization is applied. Typically set to false.
    optional<bool> needMemory; 

    // Entries related to initialization and bounds.

    /// @brief Maximal shadow capacity cost.
    optional<double> maxCapacityCost; // = 2e5;
    /// @brief Initialization suggestion for shadow capacity cost.
    optional<double> initCapacityCost; // = 2e2;

    // Entries related to randomization.

    /// @brief Initialization strategy. Normal, Uniform, or Both. Typical value is Both.
    optional<RandomInitType> initStrategy; 
    /// @brief Minimal uniform generation range. Typical value = 0.0
    optional<double> uniformMin; 
    /// @brief Maximal uniform generation range. Typical value = 2.0
    optional<double> uniformMax; // = 2.0;
    /// @brief Expected value for normal distribution. Typical value = 0.0
    optional<double> normalMean; 
    /// @brief Standard deviation for normal distribution. Typical value = 2.0
    optional<double> normalStd;
    /// @brief Probability for bernoulli distribution. Typical value = 0.01
    optional<double> bernoullyProba; 
    /// @brief Seed for reproducible randomization. Typical value = 3.
    optional<size_t> seed; 

    // Debug info.

    /// @brief CLP log level. Typical value = 3.
    optional<size_t> clpLogLevel; 
    /// @brief Whether to keep history with measurements. Typical value = false.
    optional<bool> keepStory; 

    // Common optimization method parameters.

    /// @brief Optimization algorithm family. Cutting Plane or Gradient Optimization.
    optional<OptimizationAlgo> optimizationAlgo; 
    /// @brief Maximal number of iterations. Typical value is 800.
    optional<size_t> maxSubgradientIterations;
    /// @brief Precision for subgradient methods. Typical value = 1e-5.
    optional<double> subgradientPrecision;
    /// @brief Regularization coefficient. Typical value = 0.001.
    optional<double> coeffReg; 

    // Common backend parameters.

    /// @brief Type of decision at cutoff. Typically CutOffDecisionType::BY_ITINERARY.
    optional<CutoffDecisionType> cutoffDecisionType; 
    /// @brief Method to use for CLP linear programming. Typically ClpSolutionConfig::CLP_PRIMAL.
    optional<ClpSolutionConfig> clpMethod;
    /// @brief Default precision.
    optional<double> globalPrecision;
    /// @brief Precison to treat small floats like zero.
    optional<double> zeroIgnorePrecision;

    // Cutting-plane related configurations and tweaks.

    /// @brief Minimal number of subgradient iterations in cutting plane method. Typically 15.
    optional<size_t> minSubgradientIterations;
    /// @brief Number of initialization points. Typically 15.
    optional<size_t> initPointCount; 
    /// @brief Size of deque with cutting plane points. Typically 1000.
    optional<size_t> deque_size;
    /// @brief Whether to use equations restart. Typically true.
    optional<bool> useEquationsRestart;
    /// @brief Whether to restart only once. Typically true.
    optional<bool> singleRestart; 
    /// @brief Number of iterations before restart. Typically 2000.
    optional<size_t> restartPeriod; 
    /// @brief Maximal number of immortal points in dualHistory deque. Typically 100.
    optional<size_t> immortalLimit;
    /// @brief Maximal number of immortal points in dual deque from those generated randomly.
    optional<size_t> immortalShuffleCount; // = 30;

    // GM-related configurations and tweaks.
    /// @brief Initial value of L. Typically set to 100., or 1/"gradient_step".
    optional<double> L0; 
    /// @brief Type of gradient optimizer. Current the most efficient is UFGM.
    optional<gm::GradientFamily> gradientOptimizer; 
    /// @brief UFGM prox-function. Currently L2SQUARED is the typical vallue.
    optional<gm::GmProxType> ufgmProxType; 
    /// @brief UFGM regularizer. Currently L2SQUARED is the typical value.
    optional<gm::GmRegularizerType> ufgmRegType; // L2SQUARED
};

/**
 * @brief Package with precomputed randomness.
 */
struct RandomPack {
    std::default_random_engine generator;               ///< Random engine. Turns distrubtions.
    std::normal_distribution<double> normalDist;        ///< For sampling from Normal distribution.
    std::uniform_real_distribution<double> uniformDist; ///< For sampling from Uniform distribution.
    std::bernoulli_distribution bernoullyDist;  ///< For sampling from Bernoulli disribution.
};

/**
 * @brief Entry in the dual deque - storage of points for effective cutting plane optimization.
 */
struct DualDequeInfo {
    DualVariables duals; ///< Dual variables, a point defining cutting plane line.
    bool immortal;       ///< If true, the entry is not removed from deque.
    bool checked;        ///< If checked, the entry is a checked feasible point.
};

/**
 * @brief Backend for performing optimization based on Lagrangian Relaxation.
 */
class LagrangianRelaxationBackend {

public:
    /**
     * @brief Constructor for the Lagrangian Relaxation Backend.
     * 
     * @param config Configuration for LR backend.
     */
    LagrangianRelaxationBackend(
            const LagrangianRelaxationBackendConfig& config);
    /**
     * @brief Computes dual variables, aka shadow price of capacity.
     * 
     * @param[in] state The state of trajectory evaluation.
     * @param[out] decisionManager The manager with Decision.
     * @param[out] uCoeff Place for writing Benders Decomposition coefficients, if relevant.
     * @param[out] objectiveEstimation  Place for writing objective estimation, if relevant.
     * @return The DualVariables point, optimal if converged, and approximation otherwise.
     */
    DualVariables provideDuals(
            const State& state,
            DecisionManagerPtr decisionManager,
            UCoefficients* uCoeff = nullptr,
            double* objectiveEstimation = nullptr) const;
    /**
     * @brief Makes decision for cut-off stage given dual variables storing shadow price of 
     * capacity.
     * 
     * @param[in] event Current cutoff event to process.
     * @param[in] duals Dual variables storing the shadow price of capacity.
     * @param[in] actionManager Manager with Action.
     * @param[out] decisionManager Manager with Decision. Updated.
     * @param[out] state State of trajectory evaluation. Updated.
     */
    void makeCutoffDecisionWithDuals(
            const InputData::Event& event,
            const DualVariables& duals,
            const ConstActionManagerPtr& actionManager,
            DecisionManagerPtr decisionManager,
            State* state) const;
    /**
     * @brief Set ignoreSpot boolean to a specific value. When true, this enforces the 
     * algorithm to ignore spot market.
     * 
     * @param value The value to assign.
     */
    void setIgnoreSpot(bool value) {
        ignoreSpot = value;
    }
    /**
     * @brief Provides algorithm with Ipopt-computed dual variables. A good initialization may 
     * accelerate convergence.
     * 
     * @param duals The DualVariables object computed with Fluid approximation, e.g. via Ipopt.
     */
    void provideIpoptDuals(const DualVariables& duals) {
        DualDequeInfo info;
        info.duals = duals;
        assert(duals.muVariables.size()
                    == config.inputManager->getConstData().itineraries.size());
        assert(duals.lambdaVariables.size());
        info.immortal = true;
        info.checked = false;
        std::size_t immortalCount = 0;
        for (const auto& entry: dualHistory) {
            if (entry.immortal) {
                ++immortalCount;
            }
        }
        for (auto& entry: dualHistory) {
            if (entry.immortal && immortalCount > config.immortalLimit) {
                immortalCount -= 1;
                entry.immortal = false;
            }
        }
        mean.lambdaVariables = info.duals.lambdaVariables;
        if (!mean.muVariables.size()) {
            mean = info.duals;
        }
        dualHistory.push_back(info);
    }

private:
    /**
     * @brief Logs that LR Backend is constructed.
     */
    void logLRConstruction() const;
    /**
     * @brief Logs progress in provideDuals.
     * 
     * @param decisionManager The Manager with Decision.
     * @param duals The computed dual variables to log.
     */
    void logProvideDuals(
            ConstDecisionManagerPtr decisionManager, const DualVariables& duals) const;
    /**
     * @brief Logs making cut-off decision with duals.
     * 
     * @param duals The supplied dual variables.
     */
    void logMakeCutoffDecisionWithDuals(const DualVariables& duals) const;

private:
    /// @brief Configuration for LR backend.
    LagrangianRelaxationBackendConfig config;
    /// @brief LR Index. Helps for building cutting plane optimization.
    LRIndexManagerPtr index;
    /// @brief Central duals. Helps for effective regularization in gradient methods.
    mutable DualVariables mean;
    /// @brief Deque with cutting plane pivot points (obtained at iterations, pre-initialized or
    /// randomly. Mutable because each iteration adds a point.
    mutable std::deque<DualDequeInfo> dualHistory;
    /// @brief Package with randomization. Mutable because random engine gets updated.
    mutable RandomPack randomPack;
    /// @brief If true, spot market is ignored.
    bool ignoreSpot = false;
};

} // namespace backend
} // namespace sea
