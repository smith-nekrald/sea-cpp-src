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

struct RandomPack {
    std::default_random_engine generator;
    std::normal_distribution<double> normalDist;
    std::uniform_real_distribution<double> uniformDist;
    std::bernoulli_distribution bernoullyDist;
};

struct DualDequeInfo {
    DualVariables duals;
    bool immortal;
    bool checked;
};

class LagrangianRelaxationBackend {

public:
    LagrangianRelaxationBackend(
            const LagrangianRelaxationBackendConfig& config);
    DualVariables provideDuals(
            const State& state,
            DecisionManagerPtr decisionManager,
            UCoefficients* uCoeff = nullptr,
            double* objectiveEstimation = nullptr) const;
    void makeCutoffDecisionWithDuals(
            const InputData::Event& event,
            const DualVariables& duals,
            const ConstActionManagerPtr& actionManager,
            DecisionManagerPtr decisionManager,
            State* state) const;
    void setIgnoreSpot(bool value) {
        ignoreSpot = value;
    }
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
    void logLRConstruction() const;
    void logProvideDuals(
            ConstDecisionManagerPtr decisionManger, const DualVariables& duals) const;
    void logMakeCutoffDecisionWithDuals(const DualVariables& duals) const;

private:
    LagrangianRelaxationBackendConfig config;
    LRIndexManagerPtr index;
    mutable DualVariables mean;
    mutable std::deque<DualDequeInfo> dualHistory;
    mutable RandomPack randomPack;
    bool ignoreSpot = false;
};

} // namespace backend
} // namespace sea
