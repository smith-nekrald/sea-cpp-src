#pragma once

#include "../abstract_spot_market_strategy.h"
#include "../../backend/ipopt/ipopt_backend.h"
#include "../../backend/lagrangian_relaxation/lagrangian_relaxation_backend.h"

namespace sea {
namespace strategy {

struct HybridSpotInnerConfig {
};

struct HybridSpotMarketStrategyConfig {
    SpotMarketStrategyConfig abstractConfig;
    BackendConfigHolder backendConfigs;
    HybridSpotInnerConfig innerConfig;
};

class HybridSpotMarketStrategy : public AbstractSpotMarketStrategy {
public:
    using ConfigType=HybridSpotMarketStrategyConfig;

public:
    HybridSpotMarketStrategy(const HybridSpotMarketStrategyConfig& aConfig)
        : AbstractSpotMarketStrategy(
                aConfig.abstractConfig,
                aConfig.backendConfigs,
                "hybrid_spot")
        , innerConfig(aConfig.innerConfig) {
            reset();
            lastUpdate=-1e100;
    }
    virtual ~HybridSpotMarketStrategy() {};
    void reset() override;
    virtual void setBackends(const sea::BackendHolder& holder) override;
    void updateParams(const InputData::Event& event) override;

protected:
    virtual void processCutoff(const InputData::Event& event) override;

private:
    const HybridSpotInnerConfig innerConfig;
    DualVariables duals;
};

} // namespace strategy
} // namespace sea
