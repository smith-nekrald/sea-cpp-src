/**
 * @file ipopt_allotment_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declaration and relevant structures for Ipopt Allotment Strategy.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../abstract_allotment_strategy.h"
#include "../../backend/ipopt/ipopt_backend.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

/**
 * @brief Strategy-specific inner configuration.
 */
struct IpoptInnerConfig {};

/**
 * @brief Configuration for IpoptAllotment Strategy, contains parent configuration and 
 * strategy-specific configuration.
 */
struct IpoptAllotmentStrategyConfig {
    /// @brief Configures abstract allotment strategy, parent class.
    AllotmentStrategyConfig abstractConfig;
    /// @brief Holder with backend configurations.
    BackendConfigHolder backendConfigs;
    /// @brief If true, forced allotments are fixed, and other relevant fields are computed.
    bool useForcedAllotments = false;
    /// @brief The vector with forced allotments.
    vector<bool> forcedAllotments;
    /// @brief Strategy-specific inner configuration.
    IpoptInnerConfig innerConfig;
};

/**
 * @brief Ipopt Allotment Strategy is a method to make allotments based on fluid approximation.
 */
class IpoptAllotmentStrategy: public AbstractAllotmentStrategy {
public:
    /// @brief Short naming for configuration type.
    using ConfigType=IpoptAllotmentStrategyConfig;

public:
    /**
     * @brief Constructs Ipopt Allotment Strategy.
     * 
     * @param aConfig Configuration for Ipopt Allotment Strategy.
     */
    IpoptAllotmentStrategy(const IpoptAllotmentStrategyConfig& aConfig)
            : AbstractAllotmentStrategy(
                aConfig.abstractConfig, aConfig.backendConfigs, "ipopt_allotment")
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
    /**
     * @brief Soft reset. Resets inner state without resetting backends.
     */
    void reset() override;
    /**
     * @brief Hard reset. Also resets backends. However, remembers 
     * memorizedSelection for warm-start.
     */
    void hardReset() override;
    /**
     * @brief Makes allotment decision based on fluid approximation.
     * Creates new DecisionManagerPtr and fills field related to selected allotments.
     * 
     * @return DecisionManagerPtr 
     */
    virtual DecisionManagerPtr provideAllotments() override;
    /**
     * @brief Virtual destructor for effective C++ polymorphism.
     */
    virtual ~IpoptAllotmentStrategy() {};

private:
    /**
     * @brief Logs the selected allotments.
     * 
     * @param selection The selected allotments.
     */
    void logAllotmentSelection(const vector<bool>& selection) const;
    /**
     * @brief Logs the updated decision.
     * 
     * @param result The Manager with updated Decision to update.
     */
    void logUpdatedDecision(ConstDecisionManagerPtr result) const;

private:
    /// @brief Allotment selection for warm start.
    vector<bool> memorizedSelection;
    /// @brief Enforced allotment selection.
    vector<bool> forcedAllotments;
    /// @brief Algorithm-specific configuration.
    IpoptInnerConfig innerConfig;
};

} // namespace strategy
} // namespace sea
