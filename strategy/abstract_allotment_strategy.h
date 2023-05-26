#pragma once

#include "iallotment_strategy.h"
#include "../logging/logging.h"

#include <memory>
#include <string>
#include <map>

namespace sea {
namespace strategy {


struct AllotmentStrategyConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
    std::experimental::optional<AllotmentStrategyType> type;
    std::experimental::optional<bool> storeInitialDecision; // = true;
    std::experimental::optional<double> defaultUtilizationRatio; // = 1.0;
};


class AbstractAllotmentStrategy : public IAllotmentStrategy {

public:
    AbstractAllotmentStrategy(
            const AllotmentStrategyConfig& aConfig,
            const BackendConfigHolder& aBackendConfigs,
            const std::string name);

    // Creates new DecisionManagerPtr and fills field related to selected allotments.
    virtual DecisionManagerPtr provideAllotments() override = 0;
    virtual void setUtilizationRatio(double ratio) override {
        utilizationRatio = ratio;
        if (backends.dcpBackend != nullptr) {
            backends.dcpBackend->setUtilizationRatio(ratio);
        }
        if (backends.ipoptBackend != nullptr) {
            backends.ipoptBackend->setUtilizationRatio(ratio);
        }
    }
    virtual double getUtilizationRatio() override {
        return utilizationRatio;
    }
    std::string getName() override { return name; }
    BackendHolder getBackends() override {
        return backends;
    }
    virtual void hardReset() override;
    virtual void reset() override = 0;
    virtual double getValueEstimation() override {
        return valueEstimation;
    }

    virtual ~AbstractAllotmentStrategy() {};

private:
    void logCreatedAbstractAllotmentStrategy() const;
    void logHardResetInAbstractAllotmentStrategy() const;

protected:
    const BackendConfigHolder backendConfigs;
    const AllotmentStrategyConfig config;
    const std::string name;
    double utilizationRatio;
    double valueEstimation;
    std::map<double, DecisionManagerPtr> decisionCopy;
    BackendHolder backends;
};


} // namespace strategy
} // namespace sea
