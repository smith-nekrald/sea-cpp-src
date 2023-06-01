/**
 * @file lt_spot_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Template wrapper around Spot Market Strategy for no-spot (long-term only) setting.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../../backend/ipopt/ipopt_backend.h"
#include "../abstract_spot_market_strategy.h"
#include "../../algorithm/state.h"
#include "../ipopt/ipopt_spot_strategy.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

/**
 * @brief Template Spot Market Strategy which works without spot market.
 * In some sense, long-term spot market strategy. In the language of design patterns, implements
 * a Wrapper.
 * 
 * @tparam Strategy Some Spot Market Strategy, that supports no-spot regime.
 */
template<typename Strategy>
class LTSpotMarketStrategy : public ISpotMarketStrategy {
public:
    /**
     * @brief Constructor for Long-Trem Spot Market Strategy.
     * 
     * @param aConfig Configuration for inner strategy object.
     */
    LTSpotMarketStrategy(const typename Strategy::ConfigType& aConfig)
        : strategy(new Strategy(aConfig)){
        reset();
    }
    /**
     * @brief Supplies inner strategy with allotment decision.
     * 
     * @param decisionManager Decision Manager for informing about allotment decision.
     */
    virtual void supplyAllotmentDecision(const DecisionManagerPtr& decisionManager) override {
        strategy->supplyAllotmentDecision(decisionManager);
        strategy->resetLastUpdate();
    }
    /**
     * @brief Retrieves utilization from the wrapped strategy.
     * 
     * @return The maximal allowed capacity utilization ratio.
     */
    virtual double getUtilizationRatio() {
        return strategy->getUtilizationRatio();
    }
    /**
     * @brief Builds a name for the wrapped strategy.
     * 
     * @return The strategy name.
     */
    virtual std::string getName() override {
        return "LT_modification_" + strategy->getName();
    }
    /**
     * @brief Soft-reset. Calls soft-reset of the wrapped strategy, and forces 
     * backends to ignore spot market.
     */
    virtual void reset() override {
        strategy->reset();
        auto backends = strategy->getBackends();
        if (backends.lrBackend != nullptr) {
            backends.lrBackend->setIgnoreSpot(true);
        }
        if (backends.ipoptBackend != nullptr) {
            backends.ipoptBackend->setIgnoreSpot(true);
        }
        strategy->resetLastUpdate();
    }
    /**
     * @brief Hard-resets the wrapped object, and calls soft reset from wrapper afterwards.
     */
    virtual void hardReset() override {
        strategy->hardReset();
        reset();
    }
    /**
     * @brief Delegates submitAction to the wrapped object.
     * 
     * @param action The Manager with Action.
     */
    virtual void submitAction(ConstActionManagerPtr action) override {
        strategy->submitAction(action);
    }
    /**
     * @brief Delegates makeDecision to the wrapped strategy.
     * 
     * @return Manager with decision.
     */
    virtual ConstDecisionManagerPtr makeDecision() override {
        return strategy->makeDecision();
    }
    /**
     * @brief Whether warm backends are expected and supported.
     * 
     * @return If warm backends are expected and supported, returns true. Returns false otherwise.
     */
    virtual bool needWarmBackends()  {
        return false;
    }
    /**
     * @brief Set backends method is not allowed, since managed by the wrapper.
     * Therefore, throws logic_error.
     * 
     * @param holder The holder with backends.
     */
    virtual void setBackends(const BackendHolder& holder) {
        throw std::logic_error("setBackends is not allowed for LTSpotMarketStrategy");
    }
    /**
     * @brief Gets the evaluation-optimization history. Delegates to the wrapped object.
     * 
     * @return Map from measurement name to a sequence of measured values.
     */
    virtual std::map<std::string, std::vector<double>> getStory() const override {
        return strategy->getStory();
    }
    /**
     * @brief Delegates resetLastUpdate to the wrapped object.
     */
    virtual void resetLastUpdate() {
        strategy->resetLastUpdate();
    }
    /**
     * @brief Checks whether history is tracked. Delegates to the wrapped object.
     * 
     * @return If history is tracked, returns true. Returns false otherwise.
     */
    virtual bool isStoryTracked() const override {
        return strategy->isStoryTracked();
    }
    /**
     * @brief Virtual Destructor to ensure correct C++ polymorphism.
     */
    virtual ~LTSpotMarketStrategy() {};
    /**
     * @brief Getter for the backends. Since backends are managed by the wrapper. this 
     * method throws logic_error.
     * 
     * @return Abstract class required to return Holder with Backends.
     */
    virtual BackendHolder getBackends() override {
        throw std::logic_error("getBackends is not allowed for LTSpotMarketStrategy");
    }
private:
    /// @brief Wrapped strategy object.
    std::unique_ptr<ISpotMarketStrategy> strategy;
};

} // namespace strategy
} // namespace sea

