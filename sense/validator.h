/**
 * @file validator.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines Validator - entity to verify common sense for several implemented policies.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once
#include "../backend/backend_general.h"
#include "../strategy/general_strategy.h"

namespace sea {

/**
 * @brief Configures validation.
 */
struct ValidatorConfig {
    /// @brief Manager for InputData.
    ConstInputManagerPtr input;
    /// @brief Manager for InputLinks.
    ConstLinksManagerPtr links;
    /// @brief Vector with managers for MarketData.
    std::vector<ConstMarketManagerPtr> marketSet;
};

/**
 * @brief Configures strategies which are applied during validation.
 */
struct StrategyConfigs {
    /// @brief Configures Ipopt Spot Strategy (based on fluid approximation).
    strategy::IpoptSpotMarketStrategyConfig ipoptSpot;
    /// @brief Configures Ipopt Allotment Strategy (basedon fluid approximation).
    strategy::IpoptAllotmentStrategyConfig ipoptAllotment;
    /// @brief Configures Lagrangian Relaxation-based Spot Strategy.
    strategy::LRBasedSpotMarketStrategyConfig lrSpot;
    /// @brief Configures Benders Decomposition-based Allotment Strategy.
    strategy::BendersLRAllotmentStrategyConfig bendersAllotment;
};

class Validator {
public:
    /**
     * @brief Constructor.
     *
     * @param aConfig Configuration for Validator.
     * @param aBackendConfigs  Configuration for backends.
     * @param aStrategyConfigs  Configuration for strategies.
     */
    Validator(const ValidatorConfig& aConfig,
            const BackendConfigHolder& aBackendConfigs,
            const StrategyConfigs& aStrategyConfigs)
        : config(aConfig)
        , backendConfigs(aBackendConfigs)
        , strategyConfigs(aStrategyConfigs) {
    };
    /**
     * @brief Peforms allotment selection checks. Essentially, decides on the policy
     * (bruteForceDo or randomSearch) according to problem size and calls the corresponding
     * method.
     */
    void performChecks();
    /**
     * @brief Compares Benders and Ipopt allotment policies.
     */
    void bendersCheck();

private:
    /**
     * @brief Sanity check for selected vector with allotments.
     * Checks that predicted score by Ipopt and Lagrangian Relaxation have the same sign.
     * If not, or fi complete check is requested, evaluates on a particular Market Data
     * trajectory and outputs the result.
     *
     * @param selection The vector describing selected allotments.
     * @param processAnyway If true, complete check is performed. If false, in-depth check
     * is performed if suspicious.
     */
    void processSelection(std::vector<bool> selection, bool processAnyway=false);
    /**
     * @brief Brute-force over all possible allotment selections. After selection is made,
     * if the selection is feasible, the selection is checked by processSelection.
     *
     * @param groupProductLong The product of all select-one-group sizes.
     */
    void bruteForceDo(std::size_t groupProductLong);
    /**
     * @brief Randomly makes allotment selection. After random selection is generated,
     * this selection is checked for consistency. In case the selection is consistent,
     * processSelection is called for sanity checking.
     */
    void randomSearch();

private:
    /**
     * @brief Logging method for processSelection.
     *
     * @param ipoptScore The score obtained by Ipopt objective estimation.
     * @param lrScore The score obtained by Lagrangian Relaxation objective estimation.
     * @param ipoptFullProfit The full profit obtained by evaluation of Ipopt policy on market data.
     * @param lrFullProfit The full profit obtained by evaluation of LR policy on market data.
     */
    void logScoresInProcessSelection(
        double ipoptScore, double lrScore, double ipoptFullProfit, double lrFullProfit) const;
    /**
     * @brief Logs that Validator started evaluating Benders allotments in bendersCheck() method.
     */
    void logEvaluatingIpoptInBendersCheck() const;
    /**
     * @brief Logs that Validator started evaluating Benders allotments in bendersCheck() method.
     */
    void logEvaluatingBendersInBendersCheck() const;

private:
    /// @brief Configuration for validation.
    const ValidatorConfig config;
    /// @brief Configuration for backends.
    const BackendConfigHolder backendConfigs;
    /// @brief Configuration for strategies.
    StrategyConfigs strategyConfigs;
};

/**
 * @brief Checks that selected allotments fit one-group property. I.e. at most one
 * allotment from each group is selected.
 *
 * @param selection The vector with selected allotments.
 * @param groups The vector with select-one groups.
 * @return Returns true if selection is correct, and false otherwise.
 */
bool checkSelection(const vector<bool>& selection, const vector<vector<unsigned>>& groups);

} // namespace sea
