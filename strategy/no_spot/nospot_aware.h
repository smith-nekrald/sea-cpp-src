#pragma once

#include "../abstract_allotment_strategy.h"

namespace sea {
namespace strategy {

template<typename LongStrategy>
class NospotAwareAllotmentStrategy : public IAllotmentStrategy {
public:
    NospotAwareAllotmentStrategy(
            const typename LongStrategy::ConfigType& aConfig) :
                strategy(new LongStrategy(aConfig)) {
            reset();
    }

    virtual void setUtilizationRatio(double ratio) override {
        strategy->setUtilizationRatio(ratio);
    };

    virtual double getUtilizationRatio() override {
        return strategy->getUtilizationRatio();
    }

    virtual void reset() override {
        strategy->reset();
        auto backends = strategy->getBackends();
        if (backends.lrBackend != nullptr) {
            backends.lrBackend->setIgnoreSpot(true);
        }
        if (backends.ipoptBackend != nullptr) {
            backends.ipoptBackend->setIgnoreSpot(true);
        }
        // Nospot-scenario is not supported in Benders.
        assert(backends.bendersBackend == nullptr);
        // Nospot-scenario is not supported in DCP.
        assert(backends.dcpBackend == nullptr);
    }

    virtual void hardReset() override {
        strategy->hardReset();
        reset();
    }

    virtual DecisionManagerPtr provideAllotments() override {
        return strategy->provideAllotments();
    }

    virtual BackendHolder getBackends() override {
        return strategy->getBackends();
    }

    virtual std::string getName() override {
        return strategy->getName() + "_nospot_aware";
    }
    virtual double getValueEstimation() override {
        return strategy->getValueEstimation();
    }

    virtual ~NospotAwareAllotmentStrategy() {};

private:
    std::unique_ptr<IAllotmentStrategy> strategy;
};

} // namespace strategy
} // namespace sea
