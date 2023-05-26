#pragma once

#include "../abstract_allotment_strategy.h"
#include "../../backend/det_cut_plane/dcp_backend.h"

namespace sea {
namespace strategy {

struct DCPInnerConfig {
};

struct DetCutPlaneAllotmentStrategyConfig {
    AllotmentStrategyConfig abstractConfig;
    BackendConfigHolder backendConfigs;
    DCPInnerConfig innerConfig;
};

class DetCutPlaneAllotmentStrategy : public AbstractAllotmentStrategy {
public:
    DetCutPlaneAllotmentStrategy(const DetCutPlaneAllotmentStrategyConfig& aConfig)
        : AbstractAllotmentStrategy(
                aConfig.abstractConfig,
                aConfig.backendConfigs,
                "det_cut_plane_allotment")
        , innerConfig(aConfig.innerConfig) {
    }

    virtual DecisionManagerPtr provideAllotments() override;
    virtual void reset() override;
    virtual ~DetCutPlaneAllotmentStrategy() {}

private:
    void logPrintAnswerInProvideAllotments(ConstDecisionManagerPtr decision) const;

private:
    const DCPInnerConfig innerConfig;
};

} // namespace strategy
} // namespace sea
