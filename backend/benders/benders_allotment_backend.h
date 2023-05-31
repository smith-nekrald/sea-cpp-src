/**
 * @file benders_allotment_backend.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements Backend for implementing allotment search through a heuristic based on
 * Benders Decomposition, and corresponding related API.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../../manager.h"
#include "../lagrangian_relaxation/lagrangian_relaxation_backend.h"
#include "../lagrangian_relaxation/structures.h"

#include <vector>
#include <utility>
#include <unordered_map>

namespace sea {
namespace backend {

/// @brief Simple notation for a pair with per-allotment coefficients and dual variables.
using CoefficientUInfo = std::pair<UCoefficients, DualVariables>;

/**
 * @brief Configuration for Benders Decomposition backend.
 */
struct BendersAllotmentBackendConfig {
    /// @brief Manager with InputData.
    ConstInputManagerPtr inputManager;
    /// @brief Manager with InputLinks.
    ConstLinksManagerPtr linksManager;
    /// @brief Console log-level for Cbc.
    unsigned cbcLogLevel;
    /// @brief File log-level for Cbc.
    unsigned cbcFileLogLevel;
    /// @brief If true, manually controlled efficient RAM utilization is applied.
    bool needMemory = false;
    /// @brief Precision for global convergence.
    double globalPrecision = 1e-5;
};

/**
 * @brief Vector with uCoefficients and duals. Correspond to Benders Decomposition points.
 */
struct DualStorage {
    /// @brief Vector with Benders Decomposition points.
    std::vector<CoefficientUInfo> submittedDuals;
};

/**
 * @brief Backend for making allotment decision based on Benders Decomposition.
 */
class BendersAllotmentBackend {
public:
    /**
     * @brief Constructor for Benders Allotment Backend.
     *
     * @param config Backend configuration.
     */
    BendersAllotmentBackend(const BendersAllotmentBackendConfig& config);
    /**
     * @brief Adds duals to storage. Computes uAllotment coefficients on the fly.
     *
     * @param duals The dual variable point to add.
     */
    void addDuals(const DualVariables& duals);
    /**
     * @brief Selects allotments.
     *
     * @param decisionManager Manager for Decision.
     * @return Selected allotments.
     */
    vector<bool> makeAllotments(DecisionManagerPtr decisionManager);
    /**
     * @brief Virtual Destructor for efficient C++ polymorphism.
     */
    virtual ~BendersAllotmentBackend() {}

private:
    /// @brief Configuration for benders allotment backend.
    BendersAllotmentBackendConfig config;
    /// @brief Storage for Benders points.
    DualStorage storage;
    /// @brief Stores last Cbc solution point.
    vector<double> lastSolution;
};

/**
 * @brief Recalculates uCoefficients. Calls computeSubgradienInPoint.
 *
 * @param[in] inputManager Manager with InputData.
 * @param[in] linksManager Manager with InputLinks.
 * @param[in] indexManager Manager with LR Index.
 * @param[in] state The state of trajectory evaluation.
 * @param[out] decisionManager Manager with Decision.
 * @param[out] uinfo Place to add coefficients and duals.
 */
void recalculateUCoefficientsForDeterministic(
        const ConstInputManagerPtr& inputManager,
        const ConstLinksManagerPtr& linksManager,
        const ConstLRIndexManagerPtr& indexManager,
        const State& state,
        DecisionManagerPtr decisionManager,
        vector<CoefficientUInfo>* uinfo);

/**
 * @brief Completes lastSolution (essentially, allotment selection in double format).
 * Algorithmically, computes the last element s.t. the solution gets feasible.
 *
 * @param[in] duals Dual variables and uCoefficients.
 * @param[out] lastSolution The last solution point.
 * @param[in] size The size of lastSolution.
 * @param[in] precision Desired precision.
 */
void prepareBestUSolution(const vector<CoefficientUInfo>& duals,
    double* lastSolution, unsigned size, double precision = 1e-5);

} // namespace backend
} // namespace sea
