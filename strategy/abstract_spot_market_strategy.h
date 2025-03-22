/**
 * @file abstract_spot_market_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements Abstract Spot Market Strategy.
 * Interface with default realization for some methods.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "ispot_market_strategy.h"
#include "../backend/backend_general.h"
#include "../algorithm/state.h"

#include <string>
#include <memory.h>
#include <map>
#include <vector>


namespace sea {
namespace strategy {

/**
 * @brief Configuration for the spot market strategy.
 */
struct SpotMarketStrategyConfig {
    /// @brief Type of the strategy. Helps to identify correct logging.
    std::experimental::optional<SpotStrategyType> type;
    /// @brief Pointer to a Manager with InputData.
    ConstInputManagerPtr inputManager;
    /// @brief Pointer to a Manager with InputLinks.
    ConstLinksManagerPtr linksManager;

    /// @brief The length of the update interval. Update is performed
    /// every updateInterval times. Typical value is 35.0.
    std::experimental::optional<double> updateInterval;
    /// @brief If true, warm backends are supported and expected.
    /// Warm start. Typical value equals true.
    std::experimental::optional<bool> needWarmBackends;
    /// @brief Whether update interval is moved to the real time of the firt event.
    /// If set to true, allows not to recompute parameters right after the strategy is created.
    std::experimental::optional<bool> moveUpdateInterval;

    /// @brief Expected capacity utilization ratio. Typical value is 1.0.
    std::experimental::optional<double> utilizationRatio;
    /// @brief Whether to keep history. Typical value is false.
    std::experimental::optional<bool> keepStory;
};

/**
 * @brief Provides default realization for several methods in the Spot Market Strategy Interface.
 */
class AbstractSpotMarketStrategy : public ISpotMarketStrategy {

public:
    /**
     * @brief Constructor for Abstract Spot Market Strategy.
     *
     * @param aConfig Spot Market Strategy configuration.
     * @param aBackendConfigs Configuration for backends.
     * @param aName The spot market strategy name or a part of full name.
     */
    AbstractSpotMarketStrategy(
            const SpotMarketStrategyConfig& aConfig,
            const BackendConfigHolder& aBackendConfigs,
            const std::string aName);
    /**
     * @brief Initializes inner structures.
     */
    void initialize();
    /**
     * @brief Getter for the maximal capacity utilization ratio.
     *
     * @return Maximal capacity utilization ratio.
     */
    virtual double getUtilizationRatio() override {
        return config.utilizationRatio.value();
    }
    /**
     * @brief Gets or builds spot strategy name.
     *
     * @return The name of the spot strategy.
     */
    virtual std::string getName() override {
        return name;
    };
    /**
     * @brief If spot strategy supports or expects warm-start from warm backends.
     *
     * @return If supports, returns true. False otherwise.
     */
    bool needWarmBackends() override {
        return config.needWarmBackends.value();
    }
    /**
     * @brief Retrieves backend holder.
     *
     * @return The holder with backends.
     */
    virtual BackendHolder getBackends() override {
        return backends;
    };

    /**
     * @brief Set the Backends Holder for warm-start.
     *
     * @param holder The holder with backends.
     */
    virtual void setBackends(const BackendHolder& holder) override = 0;
    /**
     * @brief Hard reset. Resets backends and calls soft reset.
     */
    virtual void hardReset() override;

    /**
     * @brief Resets last update period. Now the corresponding parameter looks like
     * the spot strategy have never updated.
     */
    virtual void resetLastUpdate() override {
        const double INF = std::numeric_limits<double>::max();
        lastUpdate = -INF;
    }
    /**
     * @brief Retrieves execution story. Story is written only if keepStory is set to true.
     *
     * @return Execution history. Map from measurement name to the array of measurements.
     */
    virtual std::map<std::string, std::vector<double>> getStory() const override  {
        return story;
    }
    /**
     * @brief Checks if history is tracked.
     *
     * @return Returns true if history is tracked. Returns false otherwise.
     */
    virtual bool isStoryTracked() const override {
        return keepStory;
    }
    /**
     * @brief Virtual destructor. To ensure that C++ polymorphism works correctly.
     */
    virtual ~AbstractSpotMarketStrategy() {};

public:
    /**
     * @brief Reads environment response for the strategic decisions. Default implementation.
     *
     * @param actionManager The Manager for an Action object (pointer).
     */
    virtual void submitAction(ConstActionManagerPtr actionManager) override;
    /**
     * @brief Default implementation for decision-making.
     *
     * @return Pointer to a Manager with Decision.
     */
    virtual ConstDecisionManagerPtr makeDecision() override;
    /**
     * @brief Supplies a Manager with Decision that already has allotment decision made
     * (perhaps through Allotment Strategy).
     *
     * @param decisionManager The manager with already made allotment decision and pre-computed
     * parameters when allotments are fixed..
     */
    virtual void supplyAllotmentDecision(const DecisionManagerPtr& decisionManager) override;
    /**
     * @brief Soft-reset. Resets inner strategy parameters without resetting backends.
     */
    virtual void reset() override = 0;

protected:
    /**
     * @brief Sub-function to process cut-off event. Pure virtual function, needs implementation.
     *
     * @param event The cut-off event to process.
     */
    virtual void processCutoff(const InputData::Event& event) = 0;
    /**
     * @brief Sub-function to process pricing event. Default implementation.
     *
     * @param event The pricing event to process.
     */
    virtual void processPricing(const InputData::Event& event);
    /**
     * @brief Sub-function to process arrival event. Default implementation.
     *
     * @param event The arrival event to process.
     */
    virtual void processArrival(const InputData::Event& event);
    /**
     * @brief Sub-function to process off-hiring at the second stage. Default implementation.
     *
     * @param event The event after which second stage with off-hiring starts.
     */
    virtual void processOffhiring(const InputData::Event& event);
    /**
     * @brief Updates parameters. Periodically recomputes parameters subject to specific
     * rules. May recompute different parameters with different periodicity. Pure virtual
     * function, needs redefinition and implementation in inherited classes.
     *
     * @param event The current event on the trajectory.
     */
    virtual void updateParams(const InputData::Event& event) = 0;

private:
    /**
     * @brief Logs that Abstract Spot Market strategy is created.
     */
    void logCreated() const;
    /**
     * @brief Logs that make decision is called.
     */
    void logCalledMakeDecision() const;
    /**
     * @brief Logs that make decision is finished.
     */
    void logFinishedMakeDecision() const;

protected:
    /// @brief Holder with backends.
    BackendHolder backends;
    /// @brief Holder with backend configurations.
    BackendConfigHolder backendConfigs;
    /// @brief Spot Market Strategy configuration.
    SpotMarketStrategyConfig config;
    /// @brief Pointer to a manager with Action.
    ConstActionManagerPtr actionManager;
    /// @brief Pointer to a manager with Decision.
    DecisionManagerPtr decisionManager;
    /// @brief State of the trajectory.
    State state;
    /// @brief Name or partial name for spot market strategy.
    std::string name;
    /// @brief the real time of the last parameter recomputation.
    /// Allows to track intervals for periodic updates.
    double lastUpdate;
    /// @brief If true, spot market strategy keeps history. Othwerise, does not keep.
    bool keepStory;
    /// @brief The history of trajectory processing. Essentially, maps from parameter
    /// name to a sequence of measurements. Parameters and measurements are strategy-specific.
    std::map<std::string, std::vector<double>> story;
};

} // namespace strategy
} // namespace sea

