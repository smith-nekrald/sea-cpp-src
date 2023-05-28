#pragma once

#include "../abstract_allotment_strategy.h"
#include "../../backend/ipopt/ipopt_backend.h"
#include "../ipopt/ipopt_allotment_strategy.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

using ZeroAllotmentStrategyConfig=IpoptAllotmentStrategyConfig;

class ZeroAllotmentStrategy: public AbstractAllotmentStrategy {
public:
    ZeroAllotmentStrategy(const IpoptAllotmentStrategyConfig& aConfig)
            : AbstractAllotmentStrategy(
                aConfig.abstractConfig, aConfig.backendConfigs, "zero_allotment") {
        reset();
        assert(config.type == AllotmentStrategyType::ZERO_IPOPT);
    }
    void reset() override;
    void hardReset() override;
    // Creates new DecisionManagerPtr and fills field related to selected allotments.
    virtual DecisionManagerPtr provideAllotments() override;
    virtual ~ZeroAllotmentStrategy() {};
};


} // namespace strategy
} // namespace sea
