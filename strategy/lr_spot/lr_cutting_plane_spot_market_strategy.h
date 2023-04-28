#pragma once

#include "../abstract_spot_market_strategy.h"
#include "../../backend/lagrangian_relaxation/lagrangian_relaxation_backend.h"
#include "../../backend/ipopt/ipopt_backend.h"
#include "../../algorithm/state.h"

namespace sea {
namespace strategy {

struct LRCuttingPlaneSpotMarketStrategyConfig {
    SpotMarketStrategyConfig abstractConfig;
    BackendConfigHolder backendConfigs;

    bool doHardUpdate = true;
    double softUpdatePeriod = 56.0;

    bool askIpoptDuals = true;
};

class LRCuttingPlaneSpotMarketStrategy : public AbstractSpotMarketStrategy {

public:
    using ConfigType=LRCuttingPlaneSpotMarketStrategy;

public:
    LRCuttingPlaneSpotMarketStrategy(const LRCuttingPlaneSpotMarketStrategyConfig& aConfig)
        : AbstractSpotMarketStrategy(
                aConfig.abstractConfig,
                aConfig.backendConfigs,
                "lr_spot")
        , doHardUpdate(aConfig.doHardUpdate)
        , askIpoptDuals(aConfig.askIpoptDuals)
        , softUpdatePeriod(aConfig.softUpdatePeriod)
        , lastSoftUpdateTime(-1e100) {
            lastUpdate = -1e100;
    }
    void reset() override;
    virtual void setBackends(const BackendHolder& holder) override;
    virtual ~LRCuttingPlaneSpotMarketStrategy() {};

protected:
    virtual void processCutoff(const InputData::Event& event) override;
    virtual void updateParams(const InputData::Event& event) override;

private:
    bool doHardUpdate;
    bool askIpoptDuals;
    double softUpdatePeriod;
    double lastSoftUpdateTime;
    DualVariables duals;
    DualVariables ipoptDuals;
};

} // namespace strategy
} // namespace sea
