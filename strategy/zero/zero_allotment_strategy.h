/**
 * @file zero_allotment_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Zero Allotment Strategy. No allotments are taken, but spot market is allowed.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../abstract_allotment_strategy.h"
#include "../../backend/ipopt/ipopt_backend.h"
#include "../ipopt/ipopt_allotment_strategy.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

/// @brief Simplified notation for re-using Ipopt Allotment Strategy configuration.
using ZeroAllotmentStrategyConfig=IpoptAllotmentStrategyConfig;

/// @brief Zero allotment strategy.  No allotments are taken, but the spot market is allowed.
class ZeroAllotmentStrategy: public AbstractAllotmentStrategy {
public:
    /**
     * @brief Constructs a new Zero Allotment Strategy object.
     * 
     * @param aConfig Configuration for zero allotment strategy.
     */
    ZeroAllotmentStrategy(const IpoptAllotmentStrategyConfig& aConfig) 
            : AbstractAllotmentStrategy(
                aConfig.abstractConfig, aConfig.backendConfigs, "zero_allotment") {
        reset();
        assert(config.type == AllotmentStrategyType::ZERO_IPOPT);
    }
    /**
     * @brief Soft-reset.
     */
    void reset() override;
    /**
     * @brief Hard-reset.
     */
    void hardReset() override;
    /**
     * @brief Fills allotments in the Decision (no allotments are taken), and the rest
     * of the expected fields.
     * 
     * @return Manager pointer with corresponding decision.
     */
    virtual DecisionManagerPtr provideAllotments() override;
    /**
     * @brief Virtual destructor for effective C++ polymorphism.
     */
    virtual ~ZeroAllotmentStrategy() {};
};

} // namespace strategy
} // namespace sea
