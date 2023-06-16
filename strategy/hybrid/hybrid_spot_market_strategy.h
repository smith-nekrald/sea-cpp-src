/**
 * @file hybrid_spot_market_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declaration for Hybrid Allotment Strategy, alongside with relevant structures and API.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <limits>

#include "../abstract_spot_market_strategy.h"
#include "../../backend/ipopt/ipopt_backend.h"
#include "../../backend/lagrangian_relaxation/lagrangian_relaxation_backend.h"

namespace sea {
namespace strategy {

/**
 * @brief Inner configuration for Hybrid Spot Market Strategy.
 */
struct HybridSpotInnerConfig {};

/**
 * @brief Configuration for Hybrid Spot Market Strategy. Both configuration for parent
 * class and for the inherited class.
 */
struct HybridSpotMarketStrategyConfig {
    /// @brief Configuration for abstract spot market strategy.
    SpotMarketStrategyConfig abstractConfig;
    /// @brief Holder with backend configurations.
    BackendConfigHolder backendConfigs;
    /// @brief Specific hybrid-related configuration.
    HybridSpotInnerConfig innerConfig;
};

/**
 * @brief Hybrid Spot Market Strategy. Essentially, the idea is to use dual variables
 * computed as side product in fluid approximation with Ipopt, and apply LR-based heuristic
 * on top of these dual variables.
 */
class HybridSpotMarketStrategy : public AbstractSpotMarketStrategy {
public:
    /// @brief Simplified notation for configuration class.
    using ConfigType=HybridSpotMarketStrategyConfig;

public:
    /**
     * @brief Constructor for Hybrid Spot Market Strategy.
     *
     * @param aConfig Configuration for Hybrid Spot Market Strategy.
     */
    HybridSpotMarketStrategy(const HybridSpotMarketStrategyConfig& aConfig)
            : AbstractSpotMarketStrategy(
                aConfig.abstractConfig, aConfig.backendConfigs, "hybrid_spot")
            , innerConfig(aConfig.innerConfig) {
        reset();
        const double INF = std::numeric_limits<double>::max();
        lastUpdate = -INF;
    }
    /**
     * @brief Virtual destructor for correct C++ polymorphism.
     */
    virtual ~HybridSpotMarketStrategy() {};
    /**
     * @brief Soft-reset. Resets inner state without resetting backends.
     */
    void reset() override;
    /**
     * @brief Sets the backends. Helps for warm-start.
     *
     * @param holder Holder with backends.
     */
    virtual void setBackends(const sea::BackendHolder& holder) override;
    /**
     * @brief Recomputes parameters with certain periodicity.
     *
     * @param event The currently relevant event.
     */
    void updateParams(const InputData::Event& event) override;

protected:
    /**
     * @brief Process cut-off stage.
     *
     * @param event The current cut-off event.
     */
    virtual void processCutoff(const InputData::Event& event) override;

private:
    /// @brief Inner algorithm-specific configuration.
    const HybridSpotInnerConfig innerConfig;
    /// @brief Currently relevant dual variables point.
    DualVariables duals;
};

} // namespace strategy
} // namespace sea
