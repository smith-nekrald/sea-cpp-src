#pragma once

#include "../../backend/ipopt/ipopt_backend.h"
#include "../abstract_spot_market_strategy.h"
#include "../../algorithm/state.h"
#include "../ipopt/ipopt_spot_strategy.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

template<typename Strategy>
class LTSpotMarketStrategy : public ISpotMarketStrategy {

public:
    LTSpotMarketStrategy(const typename Strategy::ConfigType& aConfig)
        : strategy(new Strategy(aConfig)){
        reset();
    }

    virtual void supplyAllotmentDecision(const DecisionManagerPtr& decisionManager) override {
        strategy->supplyAllotmentDecision(decisionManager);
        strategy->resetLastUpdate();
    }

    virtual double getUtilizationRatio() {
        return strategy->getUtilizationRatio();
    }

    virtual std::string getName() override {
        return "LT_modification_" + strategy->getName();
    }

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

    virtual void hardReset() override {
        strategy->hardReset();
        reset();
    }

    virtual void submitAction(ConstActionManagerPtr action) override {
        strategy->submitAction(action);
    }
    virtual ConstDecisionManagerPtr makeDecision() override {
        return strategy->makeDecision();
    }

    virtual bool needWarmBackends()  {
        return false;
    }
    virtual void setBackends(const BackendHolder& /*holder*/) {
        throw std::logic_error("setBackends is not allowed for LTSpotMarketStrategy");
    }

    virtual std::map<std::string, std::vector<double>> getStory() const override {
        return strategy->getStory();
    }

    virtual void resetLastUpdate() {
        strategy->resetLastUpdate();
    }
    virtual bool isStoryTracked() const override {
        return strategy->isStoryTracked();
    }

    virtual ~LTSpotMarketStrategy() {};

    virtual BackendHolder getBackends() override {
        throw std::logic_error("getBackends is not allowed for LTSpotMarketStrategy");
    }


private:
    std::unique_ptr<ISpotMarketStrategy> strategy;
};

} // namespace strategy
} // namespace sea

