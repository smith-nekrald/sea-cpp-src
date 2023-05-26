#pragma once

#include "ispot_market_strategy.h"
#include "../backend/backend_general.h"
#include "../algorithm/state.h"
#include "../logging/logging.h"

#include <memory>
#include <string>
#include <memory.h>
#include <map>
#include <vector>



namespace sea {
namespace strategy {

struct SpotMarketStrategyConfig {
    std::experimental::optional<SpotStrategyType> type;
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;

    std::experimental::optional<double> updateInterval; // = 35.0;
    std::experimental::optional<bool> needWarmBackends; // = true;
    std::experimental::optional<bool> moveUpdateInterval; // = true;

    // Expected capacity utilization ratio.
    std::experimental::optional<double> utilizationRatio; // = 1.0;
    std::experimental::optional<bool> keepStory; // = false
};

class AbstractSpotMarketStrategy : public ISpotMarketStrategy {

public:
    AbstractSpotMarketStrategy(
            const SpotMarketStrategyConfig& aConfig,
            const BackendConfigHolder& aBackendConfigs,
            const std::string aName);
    void initialize();

    virtual double getUtilizationRatio() override {
        return config.utilizationRatio.value();
    }

    virtual std::string getName() override {
        return name;
    };

    bool needWarmBackends() override {
        return config.needWarmBackends.value();
    }
    virtual BackendHolder getBackends() override {
        return backends;
    };

    virtual void setBackends(const BackendHolder& holder) override = 0;
    virtual void hardReset() override;

    virtual void resetLastUpdate() override {
        lastUpdate = -1e100;
    }
    virtual std::map<std::string, std::vector<double>> getStory() const override  {
        return story;
    }
    virtual ~AbstractSpotMarketStrategy() {};
    virtual bool isStoryTracked() const override {
        return keepStory;
    }

public:
    virtual void submitAction(ConstActionManagerPtr) override;
    virtual ConstDecisionManagerPtr makeDecision() override;
    virtual void supplyAllotmentDecision(const DecisionManagerPtr& decisionManager) override;
    virtual void reset() override = 0;

protected:
    virtual void processCutoff(const InputData::Event& event) = 0;
    virtual void processPricing(const InputData::Event& event);
    virtual void processArrival(const InputData::Event& event);
    virtual void processOffhiring(const InputData::Event& event);

    virtual void updateParams(const InputData::Event& event) = 0;

private:
    void logCreated() const;
    void logCalledMakeDecision() const;
    void logFinishedMakeDecision() const;

protected:
    BackendHolder backends;
    BackendConfigHolder backendConfigs;

    SpotMarketStrategyConfig config;
    ConstActionManagerPtr actionManager;
    DecisionManagerPtr decisionManager;
    State state;

    std::string name;

    double lastUpdate;

    bool keepStory;
    std::map<std::string, std::vector<double>> story;
};

} // namespace strategy
} // namespace sea

