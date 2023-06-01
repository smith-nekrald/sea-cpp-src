/**
 * @file nospot_aware.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Template implementation for long-term only nospot-aware allotment strategy.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../abstract_allotment_strategy.h"

namespace sea {
namespace strategy {

/**
 * @brief Wrapper around allotment strategy to enforce no-spot setting (long-term only).
 * 
 * @tparam LongStrategy Type of the wrapped allotment strategy.
 */
template<typename LongStrategy>
class NospotAwareAllotmentStrategy : public IAllotmentStrategy {
public:
    /**
     * @brief Constructor for no-spot-aware long-term only allotment strategy.
     * 
     * @param aConfig Configuration for the wrapped strategy.
     */
    NospotAwareAllotmentStrategy(
            const typename LongStrategy::ConfigType& aConfig) :
                strategy(new LongStrategy(aConfig)) {
            reset();
    }
    /**
     * @brief Set the maximal allowed capacity Utilization Ratio. Delegates to the wrapped
     * object.
     * 
     * @param ratio The maximal allowed capacity utilization ratio.
     */
    virtual void setUtilizationRatio(double ratio) override {
        strategy->setUtilizationRatio(ratio);
    };
    /**
     * @brief Gets the maximal allowed capacity Utilization Ratio. Delegates to the
     * wrapped object.
     * 
     * @return The maximal allowed capacity utilization ratio.
     */
    virtual double getUtilizationRatio() override {
        return strategy->getUtilizationRatio();
    }
    /**
     * @brief Soft-reset. Delegates soft-reset, and ensures that backends are 
     * set to the no-spot (long-term only) mode.
     */
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
    /**
     * @brief Hard reset. Delegates to the wrapped strategy and calls soft-reset.
     */
    virtual void hardReset() override {
        strategy->hardReset();
        reset();
    }
    /**
     * @brief Method to make allotment decision. Delegates to the wrapped strategy.
     * 
     * @return Decision Manager with allotment decision and other relevant fields.
     */
    virtual DecisionManagerPtr provideAllotments() override {
        return strategy->provideAllotments();
    }
    /**
     * @brief Method to get backends. Delegates to the wrapped object.
     * 
     * @return Holder with backends.
     */
    virtual BackendHolder getBackends() override {
        return strategy->getBackends();
    }
    /**
     * @brief Creates a name for long-term no-spot-aware allotment strategy.
     * 
     * @return The created name for the wrapper.
     */
    virtual std::string getName() override {
        return strategy->getName() + "_nospot_aware";
    }
    /**
     * @brief Delegates value estimation to the wrapped strategy.
     * 
     * @return The estimated profit.
     */
    virtual double getValueEstimation() override {
        return strategy->getValueEstimation();
    }
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~NospotAwareAllotmentStrategy() {};

private:
    /// @brief The wrapped allotment strategy.
    std::unique_ptr<IAllotmentStrategy> strategy;
};

} // namespace strategy
} // namespace sea
