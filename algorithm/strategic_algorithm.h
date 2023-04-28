#pragma once

#include <memory>

#include "ialgorithm.h"
#include "state.h"
#include "../strategy/general_strategy.h"
#include "../interaction/iinteractor.h"
#include "../logging/logging.h"

namespace sea {

namespace algo {

struct StrategicAlgorithmConfig {
    strategy::IAllotmentStrategyPtr allotmentStrategy;
    strategy::ISpotMarketStrategyPtr spotMarketStrategy;
};


class StrategicAlgorithm : public IAlgorithm {

public:
    explicit StrategicAlgorithm(const StrategicAlgorithmConfig& aConfig);

    // Functions makeDecision and submitAction are virtual,
    // but default implementation is provided.
    // (And really it is not going to be changed.)
    virtual ConstDecisionManagerPtr makeDecision() override;
    virtual void submitAction(ConstActionManagerPtr) override;
    virtual void reset() override;
    virtual void synchronizeStrategies() override;
    virtual std::string getName() override {
        return "StrategicAlgorithm:<" + config.allotmentStrategy->getName() + "," +
            config.spotMarketStrategy->getName() + ">";
    };

    virtual std::map<std::string, std::vector<double>> getStory() const override {
        auto story = config.spotMarketStrategy->getStory();
        if (config.spotMarketStrategy->isStoryTracked()) {
            story["allotment_value"].push_back(config.allotmentStrategy->getValueEstimation());
        }
        return story;
    }

    virtual ~StrategicAlgorithm() {};

protected:

    StrategicAlgorithmConfig config;
    bool allotmentsAsked = false;

};

typedef std::shared_ptr<StrategicAlgorithm> StrategicAlgorithmPtr;

} // namespace algo

} // namespace sea
