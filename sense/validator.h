#include "../backend/backend_general.h"
#include "../strategy/general_strategy.h"

namespace sea {

struct ValidatorConfig {
    ConstInputManagerPtr input;
    ConstLinksManagerPtr links;
    std::vector<ConstMarketManagerPtr> marketSet;
};

struct StrategyConfigs {
    strategy::IpoptSpotMarketStrategyConfig ipoptSpot;
    strategy::IpoptAllotmentStrategyConfig ipoptAllotment;
    strategy::LRCuttingPlaneSpotMarketStrategyConfig lrSpot;
    strategy::BendersLRAllotmentStrategyConfig bendersAllotment;
};

class Validator {
public:
    Validator(const ValidatorConfig& aConfig,
            const BackendConfigHolder& aBackendConfigs,
            const StrategyConfigs& aStrategyConfigs)
        : config(aConfig)
        , backendConfigs(aBackendConfigs)
        , strategyConfigs(aStrategyConfigs) {
    };

    void performChecks();
    void bendersCheck();

private:
    void processSelection(std::vector<bool> selection, bool processAnyway=false);
    void bruteForceDo(std::size_t groupProductLong);
    void randomSearch();

private:
    const ValidatorConfig config;
    const BackendConfigHolder backendConfigs;
    StrategyConfigs strategyConfigs;

};

} // namespace sea
