#pragma once

#include "../abstract_allotment_strategy.h"
#include "../../backend/ipopt/ipopt_backend.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

struct IpoptInnerConfig {

};

struct IpoptAllotmentStrategyConfig {
    AllotmentStrategyConfig abstractConfig;
    BackendConfigHolder backendConfigs;
    vector<bool> forcedAllotments;
    bool useForcedAllotments = false;

    IpoptInnerConfig innerConfig;
};

class IpoptAllotmentStrategy: public AbstractAllotmentStrategy {
public:
    using ConfigType=IpoptAllotmentStrategyConfig;

public:
    IpoptAllotmentStrategy(const IpoptAllotmentStrategyConfig& aConfig)
            : AbstractAllotmentStrategy(
                    aConfig.abstractConfig,
                    aConfig.backendConfigs,
                    "ipopt_allotment")
            , innerConfig(aConfig.innerConfig) {
        reset();
        assert(config.type == AllotmentStrategyType::IPOPT);
        forcedAllotments.clear();
        if (aConfig.useForcedAllotments) {
            assert(aConfig.forcedAllotments.size() ==
                    config.inputManager->getConstData().allotments.size());
            forcedAllotments = aConfig.forcedAllotments;
            memorizedSelection = forcedAllotments;
        }
    }
    void reset() override;
    void hardReset() override;
    // Creates new DecisionManagerPtr and fills field related to selected allotments.
    virtual DecisionManagerPtr provideAllotments() override;
    virtual ~IpoptAllotmentStrategy() {};

private:
    vector<bool> memorizedSelection;
    vector<bool> forcedAllotments;

    IpoptInnerConfig innerConfig;
};


} // namespace strategy
} // namespace sea
