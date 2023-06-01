/**
 * @file lr_spot_market_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declaration and relevant structures for Lagrangian Relaxation Spot Market Strategy.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../abstract_spot_market_strategy.h"
#include "../../backend/lagrangian_relaxation/lagrangian_relaxation_backend.h"
#include "../../backend/ipopt/ipopt_backend.h"
#include "../../algorithm/state.h"

namespace sea {
namespace strategy {

/**
 * @brief Configures Lagrangian Relaxation Spot Market Strategy.
 */
struct LRBasedSpotMarketStrategyConfig {
    /// @brief Configuration for abstract spot market strategy (parent class).
    SpotMarketStrategyConfig abstractConfig;
    /// @brief Holder with backend configurations.
    BackendConfigHolder backendConfigs;
    /// @brief Whether to do hard update at all (recompute fluid approximation).
    bool doHardUpdate = true;
    /// @brief Period for soft update (keep fluid approximation values, but optimize over duals).
    double softUpdatePeriod = 56.0;
    /// @brief Whether to ask for Ipopt dual variables at initialization point.
    bool askIpoptDuals = true;
};

class LRBasedSpotMarketStrategy : public AbstractSpotMarketStrategy {
public:
    /// @brief Simplified naming for configuration type.
    using ConfigType=LRBasedSpotMarketStrategyConfig;

public:
    /**
     * @brief Constructor for LR Spot Market Strategy.
     * 
     * @param aConfig LR Spot Market Strategy Configuration.
     */
    LRBasedSpotMarketStrategy(const LRBasedSpotMarketStrategyConfig& aConfig)
        : AbstractSpotMarketStrategy(
                aConfig.abstractConfig,
                aConfig.backendConfigs,
                "lr_spot")
        , doHardUpdate(aConfig.doHardUpdate)
        , askIpoptDuals(aConfig.askIpoptDuals)
        , softUpdatePeriod(aConfig.softUpdatePeriod) {
            const double INF = std::numeric_limits<double>::max();
            lastSoftUpdateTime = -INF;
            lastUpdate = -INF;
    }
    /**
     * @brief Soft-reset. Resets inner state without resettting backends.
     */
    void reset() override;
    /**
     * @brief Setter for backends.
     * 
     * @param holder The holder with backends to use.
     */
    virtual void setBackends(const BackendHolder& holder) override;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~LRBasedSpotMarketStrategy() {};

protected:
    /**
     * @brief Strategy-specific implementation for processing cut-off events.
     * 
     * @param event The current cut-off event at the evaluation trajectory.
     */
    virtual void processCutoff(const InputData::Event& event) override;
    /**
     * @brief Strategy-specific update for parameters. Essentially, two type of updates
     * are possible: hard update is about recomputing fluid approximation, and soft update
     * is about keeping fluid values fixed and recomputing dual variables.
     * 
     * @param event The current event at the evaluation trajectory.
     */
    virtual void updateParams(const InputData::Event& event) override;

private:
    /**
     * @brief Logs dual variables in the updateParams function.
     * 
     * @param event The current event in the evaluation trajectory.
     */
    void logDualsInUpdateParams(const InputData::Event& event) const;

private:
    /// @brief Whether to do hard update (recompute fluid approximation).
    bool doHardUpdate;
    /// @brief Whether to ask Ipopt for dual values for initialization.
    bool askIpoptDuals;
    /// @brief Update period for recomputing dual variables (soft update).
    double softUpdatePeriod;
    /// @brief Last time of soft update (recomputing dual variables).
    double lastSoftUpdateTime;
    /// @brief Currently relevant dual variables.
    DualVariables duals;
    /// @brief Dual variables, provided by Ipopt fluid approximation.
    DualVariables ipoptDuals;
};

} // namespace strategy
} // namespace sea
