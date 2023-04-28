#pragma once

#include "../manager.h"
#include "../interaction/iinteractor.h"
#include "../algorithm/state.h"
#include "../backend/backend_general.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

class ISpotMarketStrategy : public IInteractor {
public:
    ISpotMarketStrategy() {};

    virtual void supplyAllotmentDecision(const DecisionManagerPtr& decisionManager) = 0;
    virtual std::string getName() = 0;

    virtual double getUtilizationRatio() = 0;

    virtual void reset() = 0;
    virtual void hardReset() = 0;
    virtual void resetLastUpdate() = 0;

    virtual void submitAction(ConstActionManagerPtr) override = 0;
    virtual ConstDecisionManagerPtr makeDecision() override = 0;

    virtual bool needWarmBackends() = 0;
    virtual void setBackends(const BackendHolder& holder) = 0;

    virtual ~ISpotMarketStrategy() {};

    virtual BackendHolder getBackends() = 0;
    virtual std::map<std::string, std::vector<double>> getStory() const = 0;
    virtual bool isStoryTracked() const = 0;
};

using ISpotMarketStrategyPtr=std::shared_ptr<ISpotMarketStrategy>;

} // namespace strategy
} // namespace sea
