#pragma once

#include "../../backend/ipopt/ipopt_backend.h"
#include "../abstract_spot_market_strategy.h"
#include "../../algorithm/state.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

struct IpoptSpotMarketStrategyConfig {
    SpotMarketStrategyConfig abstractConfig;
    BackendConfigHolder backendConfigs;
};

class IpoptSpotMarketStrategy : public AbstractSpotMarketStrategy {
public:
    using ConfigType=IpoptSpotMarketStrategyConfig;

public:
    IpoptSpotMarketStrategy(const IpoptSpotMarketStrategyConfig& aConfig)
        : AbstractSpotMarketStrategy(
                aConfig.abstractConfig,
                aConfig.backendConfigs,
                "ipopt_spot") {
        reset();
    }
    void reset() override;
    void updateParams(const InputData::Event& event) override;
    void setBackends(const BackendHolder& holder) override;
    virtual ~IpoptSpotMarketStrategy() {};

protected:
    virtual void processCutoff(const InputData::Event& event) override;

private:
    void logEnteredUpdateParams(bool needUpdate) const;
    void logLeftUpdateParams() const;
};

} // namespace strategy
} // namespace sea

