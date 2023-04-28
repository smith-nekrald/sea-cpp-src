#pragma once

#include "../abstract_allotment_strategy.h"
#include "../../backend/ipopt/ipopt_backend.h"
#include "../ipopt/ipopt_allotment_strategy.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

struct NullAllotmentStrategyConfig {
    AllotmentStrategyConfig abstractConfig;
    BackendConfigHolder backendConfigs;
};



class NullAllotmentStrategy: public AbstractAllotmentStrategy {
public:
    NullAllotmentStrategy(const NullAllotmentStrategyConfig& aConfig)
        : AbstractAllotmentStrategy(
                aConfig.abstractConfig,
                aConfig.backendConfigs,
                "null_allotment") {
            reset();
            assert(config.type == AllotmentStrategyType::ZERO_NULL);
    }
    void reset() override;
    // Creates new DecisionManagerPtr and fills field related to selected allotments.
    virtual DecisionManagerPtr provideAllotments() override;
    virtual ~NullAllotmentStrategy() {};
};


} // namespace strategy
} // namespace sea
