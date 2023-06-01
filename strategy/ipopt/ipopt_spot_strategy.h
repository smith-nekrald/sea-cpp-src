/**
 * @file ipopt_spot_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares Ipopt Spot Market Strategy, relevant structures and API.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../../backend/ipopt/ipopt_backend.h"
#include "../abstract_spot_market_strategy.h"
#include "../../algorithm/state.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

/**
 * @brief Configuration for Ipopt Spot Market Strategy.
 */
struct IpoptSpotMarketStrategyConfig {
    /// @brief Configuration component for Spot Market Strategy.
    SpotMarketStrategyConfig abstractConfig;
    /// @brief Holder with backend configuration.
    BackendConfigHolder backendConfigs;
};

/**
 * @brief Ipopt Spot Market Strategy - strategy to make spot-market decisions based on 
 * fluid approximation approach.
 */
class IpoptSpotMarketStrategy : public AbstractSpotMarketStrategy {
public:
    /// @brief Simple naming for configuration structure.
    using ConfigType=IpoptSpotMarketStrategyConfig;

public:
    /**
     * @brief Constructor for a new Ipopt Spot Market Strategy.
     * 
     * @param aConfig Configuration for Ipopt Spot Market Strategy.
     */
    IpoptSpotMarketStrategy(const IpoptSpotMarketStrategyConfig& aConfig)
        : AbstractSpotMarketStrategy(
                aConfig.abstractConfig, aConfig.backendConfigs, "ipopt_spot") {
        reset();
    }
    /**
     * @brief Soft reset. Resets inner state without resetting backends.
     */
    void reset() override;
    /**
     * @brief Recomputes parameters, if relevant.
     * 
     * @param event Current event on the evaluation trajectory.
     */
    void updateParams(const InputData::Event& event) override;
    /**
     * @brief Provides strategy with backends.
     * 
     * @param holder Holder with backends.
     */
    void setBackends(const BackendHolder& holder) override;
    /**
     * @brief Virtual destructor for correct C++ polymorphism.
     */
    virtual ~IpoptSpotMarketStrategy() {};

protected:
    /**
     * @brief Strategy-speficific implementation for processing cut-off events.
     * 
     * @param event The current cut-off event to process.
     */
    virtual void processCutoff(const InputData::Event& event) override;

private:
    /**
     * @brief Logs that updateParams is started.
     * 
     * @param needUpdate If update is needed.
     */
    void logEnteredUpdateParams(bool needUpdate) const;
    /**
     * @brief Logs that updateParams is over.
     */
    void logLeftUpdateParams() const;
};

} // namespace strategy
} // namespace sea

