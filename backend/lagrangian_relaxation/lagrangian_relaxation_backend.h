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

enum class ClpSolutionConfig {
    CLP_PRIMAL,
    CLP_DUAL
};

enum class CutoffDecisionType {
    BY_ITINERARY,
    BY_EVENT
};

enum class RandomInitType {
    RANDOM_NORMAL,
    RANDOM_UNIFORM,
    RANDOM_BOTH
};

enum class OptimizationAlgo {
    CUTTING_PLANE,
    GM
};

struct LagrangianRelaxationBackendConfig {
    // Common entries related to problem structure.
    ConstInputManagerPtr inputManager = nullptr;
    ConstLinksManagerPtr linksManager = nullptr;
    optional<bool> needMemory; //  = false;

    // Entries related to initialization and bounds.
    optional<double> maxCapacityCost; // = 2e5;
    optional<double> initCapacityCost; // = 2e2;

    // Entries related to randomization.
    optional<RandomInitType> initStrategy; // = RandomInitType::RANDOM_BOTH;
    optional<double> uniformMin; // = 0.0;
    optional<double> uniformMax; // = 2.0;
    optional<double> normalMean; // = 0.0;
    optional<double> normalStd;  // = 2.0;
    optional<double> bernoullyProba; // = 0.01;
    optional<size_t> seed; // = 7;

    // Debug info.
    optional<size_t> clpLogLevel; // = 3;
    optional<bool> keepStory; // = false;

    // Common optimization method parameters.
    optional<OptimizationAlgo> optimizationAlgo; // cutting plane now
    optional<size_t> maxSubgradientIterations; // = 800;
    optional<double> subgradientPrecision; // = 1e-5;
    optional<double> coeffReg; // = 0.001;

    // Common backend parameters.
    optional<CutoffDecisionType> cutoffDecisionType; // = CutoffDecisionType::BY_ITINERARY;
    optional<ClpSolutionConfig> clpMethod; // = ClpSolutionConfig::CLP_PRIMAL;
    optional<double> globalPrecision; // = 1e-5;
    optional<double> zeroIgnorePrecision; // = 1e-10;

    // Cutting-plane related configurations and tweaks.
    optional<size_t> minSubgradientIterations; //  = 15;
    optional<size_t> initPointCount; // = 15;
    optional<size_t> deque_size; // = 1000;
    optional<bool> useEquationsRestart; // = true;
    optional<bool> singleRestart; // = true;
    optional<size_t> restartPeriod; // = 2000;
    optional<size_t> immortalLimit; // = 100;
    optional<size_t> immortalShuffleCount; // = 30;

    // GM-related configurations and tweaks.
    optional<double> L0; // = 100., 1/"gradient_step";
    optional<gm::GradientFamily> gradientOptimizer; // = ::UFGM
    optional<gm::GmProxType> ufgmProxType; // L2SQUARED
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
        assert(
                duals.muVariables.size()
                    == config.inputManager->getConstData().itineraries.size()
        );
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
    LagrangianRelaxationBackendConfig config;
    LRIndexManagerPtr index;
    mutable DualVariables mean;
    mutable std::deque<DualDequeInfo> dualHistory;
    mutable RandomPack randomPack;
    bool ignoreSpot = false;
};

} // namespace backend
} // namespace sea
