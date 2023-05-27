/**
 * @file ispot_market_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares interface to a spot market strategy.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../manager.h"
#include "../interaction/iinteractor.h"
#include "../algorithm/state.h"
#include "../backend/backend_general.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

/**
 * @brief Spot Market Strategy interface. Spot Market Strategy is initially provided with allotment
 * decision, then responds to events via makeDecision() and receives environment reaction via
 * submitAction().
 */
class ISpotMarketStrategy : public IInteractor {
public:
    /**
     * @brief Construct new Spot Market Strategy.
     */
    ISpotMarketStrategy() {};
    /**
     * @brief Provide Spot Market Strategy with information about made allotment decision.
     *
     * @param decisionManager The manager with decision implemented by some Allotment Strategy.
     */
    virtual void supplyAllotmentDecision(const DecisionManagerPtr& decisionManager) = 0;
    /**
     * @brief Creates strategy name.
     *
     * @return The name of the spot market strategy.
     */
    virtual std::string getName() = 0;

    /**
     * @brief Getter for the capacity utilization ratio parameter.
     *
     * @return The maximal capacity utilization ratio.
     */
    virtual double getUtilizationRatio() = 0;

    /**
     * @brief Resets spot market strategy.
     */
    virtual void reset() = 0;
    /**
     * @brief Hard-resets spot market strategy. Resets backends and then calls reset().
     */
    virtual void hardReset() = 0;
    /**
     * @brief Resets the last update. Typically sets the last update time to -INF.
     */
    virtual void resetLastUpdate() = 0;

    /**
     * @brief Method to supply the strategy with last environment Action.
     */
    virtual void submitAction(ConstActionManagerPtr) override = 0;
    /**
     * @brief Methot to ask spot market strategy to make a decision..
     *
     * @return Decision manager with made decision.
     */
    virtual ConstDecisionManagerPtr makeDecision() override = 0;

    /**
     * @brief Checker if strategy accepts warm backends.
     *
     * @return True if warm backends are supported, false otherwise.
     */
    virtual bool needWarmBackends() = 0;
    /**
     * @brief Set the Backend Holder.
     *
     * @param holder The holder with backends.
     */
    virtual void setBackends(const BackendHolder& holder) = 0;

    /**
     * @brief Getter for backend holder.
     *
     * @return Backend holder with backends.
     */
    virtual BackendHolder getBackends() = 0;
    /**
     * @brief Getther for the history.
     *
     * @return The history of the strategy performance.
     */
    virtual std::map<std::string, std::vector<double>> getStory() const = 0;
    /**
     * @brief Check if history is tracked.
     *
     * @return If tracked, true. If not, false.
     */
    virtual bool isStoryTracked() const = 0;

    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ISpotMarketStrategy() {};
};

/// @brief Simple notation for shared pointer to spot market strategy.
using ISpotMarketStrategyPtr=std::shared_ptr<ISpotMarketStrategy>;

} // namespace strategy
} // namespace sea
