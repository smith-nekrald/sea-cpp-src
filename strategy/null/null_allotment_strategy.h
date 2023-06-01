/**
 * @file null_allotment_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements Null Allotment Strategy - no allotments are taken.
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

/**
 * @brief Configuration for Null Allotment Strategy.
 */
struct NullAllotmentStrategyConfig {
    /// @brief Component for configuring abstract allotment strategy.
    AllotmentStrategyConfig abstractConfig;
    /// @brief Holder with backend configurations.
    BackendConfigHolder backendConfigs;
};


/**
 * @brief Null Allotment Strategy: no allotments are taken. However, normal spot market
 * processing is allowed, and therefore the rest of the parameters are optimized accordingly.
 */
class NullAllotmentStrategy: public AbstractAllotmentStrategy {
public:
    /**
     * @brief Constructor for a Null Allotment Strategy.
     * 
     * @param aConfig Configuration for Null Allotment Strategy.
     */
    NullAllotmentStrategy(const NullAllotmentStrategyConfig& aConfig)
            : AbstractAllotmentStrategy(
                aConfig.abstractConfig, aConfig.backendConfigs, "null_allotment") {
        reset();
        assert(config.type == AllotmentStrategyType::ZERO_NULL);
    }
    /**
     * @brief Soft-reset.
     */
    void reset() override;
    /**
     * @brief Fills allotments in the Decision (no allotments are taken), and the rest
     * of the expected fields.
     * 
     * @return Manager with corresponding Decision.
     */
    virtual DecisionManagerPtr provideAllotments() override;
    /**
     * @brief Virtual destructor for effective C++ polymorphism.
     */
    virtual ~NullAllotmentStrategy() {};
};


/**
 * @brief Template method to fill a vector with zeroes.
 * 
 * @tparam Type The type of the vector entry.
 * @param data The data to fill with zeroes.
 */
template<typename Type>
void fillZero(std::vector<Type>& data) {
    for (std::size_t ind = 0; ind < data.size(); ++ind) {
        data[ind] = 0;
    }
}

} // namespace strategy
} // namespace sea
