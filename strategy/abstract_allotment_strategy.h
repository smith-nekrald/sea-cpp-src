/**
 * @file abstract_allotment_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares Abstract Allotment Strategy.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "iallotment_strategy.h"
#include "../logging/logging.h"

#include <memory>
#include <string>
#include <map>

namespace sea {
namespace strategy {

/**
 * @brief Configures allotment strategy.
 */
struct AllotmentStrategyConfig {
    /// @brief Manager with read InputData.
    ConstInputManagerPtr inputManager;
    /// @brief Manager with ready-to-use InputLinks.
    ConstLinksManagerPtr linksManager;
    /// @brief Type of allotment strategy. Helps to create the right type of logging.
    std::experimental::optional<AllotmentStrategyType> type;
    /// @brief Whether initial decision needs to be stored. For memory-efficient cases,
    /// not storing is sometimes helpful. Usually set to true.
    std::experimental::optional<bool> storeInitialDecision; 
    /// @brief Sometimes it helps to reduce capacity artificially and use only fraction of the
    /// available capacity. This parameter accounts for defualt capacity utilization ratio. 
    /// Usually set to 1.0.
    std::experimental::optional<double> defaultUtilizationRatio;
};


/**
 * @brief Abstract Allotment Strategy is an abstract class providing 
 * several default implementations for the Allotment Strategy Interface.
 */
class AbstractAllotmentStrategy : public IAllotmentStrategy {

public:
    /**
     * @brief Construct a new Abstract Allotment Strategy.
     * 
     * @param aConfig Configuration for the Allotment Strategy.
     * @param aBackendConfigs Structure with backend configurations holder.
     * @param name The name of the Allotment Strategy.
     */
    AbstractAllotmentStrategy(
            const AllotmentStrategyConfig& aConfig,
            const BackendConfigHolder& aBackendConfigs,
            const std::string name);

    /**
     * @brief Creates new DecisionManagerPtr and fills fields related to the selected allotments.
     * 
     * @return Decision Manager with selected allotments.
     */
    virtual DecisionManagerPtr provideAllotments() override = 0;
    /**
     * @brief Set the Maximal Capacity Utilization Ratio;
     * 
     * @param ratio The capacity utilization ratio to assign.
     */
    virtual void setUtilizationRatio(double ratio) override {
        utilizationRatio = ratio;
        if (backends.dcpBackend != nullptr) {
            backends.dcpBackend->setUtilizationRatio(ratio);
        }
        if (backends.ipoptBackend != nullptr) {
            backends.ipoptBackend->setUtilizationRatio(ratio);
        }
    } 
    /**
     * @brief Getter for the maximal capacity utilization ratio.
     * 
     * @return double The maximal capacity utilization ratio.
     */
    virtual double getUtilizationRatio() override {
        return utilizationRatio;
    }
    /**
     * @brief Method to get or build allotment strategy name.
     * 
     * @return The name of the allotment strategy.
     */
    std::string getName() override { return name; }
    /**
     * @brief Method to get the holder with backends.
     * 
     * @return Backend Holder with corresponding backends.
     */
    BackendHolder getBackends() override {
        return backends;
    }
    /**
     * @brief Hard reset. Resets backends and calls soft reset.
     */
    virtual void hardReset() override;
    /**
     * @brief Soft reset. Resets strategy-specific parameters without resetting backends.
     */
    virtual void reset() override = 0;
    /**
     * @brief Gets the objective value estimation. Side product e.g. for Ipopt or LR optimization.
     * 
     * @return The objective value estimation.
     */
    virtual double getValueEstimation() override {
        return valueEstimation;
    }
    /**
     * @brief Virtual destructor to ensure that C++ polymorphism works.
     */
    virtual ~AbstractAllotmentStrategy() {};

private:
    /**
     * @brief Logs that abstract allotment strategy is created.
     */
    void logCreatedAbstractAllotmentStrategy() const;
    /**
     * @brief Logs that hard reset is called in abstract allotment strategy.
     */
    void logHardResetInAbstractAllotmentStrategy() const;

protected:
    /// @brief Holder with backend configurations.
    const BackendConfigHolder backendConfigs;
    /// @brief Configuration for the allotment strategy.
    const AllotmentStrategyConfig config;
    /// @brief The name (or partial name) of the allotment strategy.
    const std::string name;
    /// @brief Maximal capacity utilization ratio.
    double utilizationRatio;
    /// @brief Estimation for the total revenue, side product of optimization.
    double valueEstimation;
    /// @brief Maps utilization ratio to a copy of Decision Manager 
    /// to process when such ratio is fixed.
    std::map<double, DecisionManagerPtr> decisionCopy;
    /// @brief Holder with backends.
    BackendHolder backends;
};


} // namespace strategy
} // namespace sea
