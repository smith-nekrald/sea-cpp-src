/**
 * @file estimator.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements Estimator - entity to compute upper bounds on the 
 * possible profits from allotment and spot market.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../input/input_data.h"
#include "../input/market_data.h"
#include "../input/input_links.h"

#include <cassert>
#include <algorithm>
#include <ostream>

namespace sea {

/**
 * @brief Structure to hold the profit estimation.
 */
struct RevenueEstimate {
    /// @brief Part from allotment market.
    double allotment = 0;
    /// @brief Part from spot market.
    double spotMarket = 0;

    /**
     * @brief Operator to output into stream.
     * 
     * @param[out] out Stream to write into.
     * @param[in] estimate The entry to export.
     * @return The stream for further usage.
     */
    friend std::ostream& operator<<(
            std::ostream& out, const RevenueEstimate& estimate);
};

/**
 * @brief Interface for profit estimation. Intended usage is for finding upper bounds
 * derived from knowledge with perfect information. 
 */
class BaseEstimator {
public:
    /** 
     * @brief Calculates profit upper bound.
     * @param input The structure with input statistics.
     * @param links The structure over input to simplify indexation.
     * @param market The structure with perfect information on market realization.
     * @return Estimation for the profit. Upper bound.
     */
    RevenueEstimate calcUpperBound(
            const InputData& input, const InputLinks& links, const MarketData& market) const;

    /**
     * @brief Virtual destructor for correct C++ polymorphism.
     */
    virtual ~BaseEstimator() {}

protected:
    /**
     * @brief Estimates upper bound on the profit from allotments.
     * 
     * @param input The InputData structure with input statistics.
     * @param links The structure over input to simplify indexation.
     * @param market The perfect information on the market trajectory.
     * @return double The upper bound on the profit from allotments.
     */
    virtual double estimateAllotments(
        const InputData& input, const InputLinks& links, const MarketData& market) const;
    /**
     * @brief Estimates upper bound on the profit from spot market.
     * 
     * @param input The InputData structure with input statistics.
     * @param links The structure over input to simplify indexation.
     * @param market The perfect information on the market trajectory.
     * @return double The upper bound on the profit from spot market.
     */
    virtual double estimateSpotMarket(
        const InputData& input, const InputLinks& links, const MarketData& market) const;
    /**
     * @brief Fills allotmentProfitEstimaton - estimation for maximal profit per each allotment.
     * 
     * @param input The InputData structure with input statistics.
     * @param links The structure over input to simplify indexation.
     * @param market The perfect information on the market trajectory.
     * @return Vector, each entry corresponds to a maximal profit from allotment estimation.
     */
    std::vector<double> estimateEachAllotment(
        const InputData& input, const InputLinks& links, const MarketData& market) const;
};

class SmartEstimator : public BaseEstimator {
private:
    /**
     * @brief Estimates upper bound on the profit from spot market.
     * 
     * @param input The InputData structure with input statistics.
     * @param links The structure over input to simplify indexation.
     * @param market The perfect information on the market trajectory.
     * @return double The upper bound on the profit from spot market.
     */
    virtual double estimateSpotMarket(
        const InputData& input, const InputLinks& links, const MarketData& market) const override;
    /**
     * @brief Estimates upper bound on the profit from allotments.
     * 
     * @param input The InputData structure with input statistics.
     * @param links The structure over input to simplify indexation.
     * @param market The perfect information on the market trajectory.
     * @return double The upper bound on the profit from allotments.
     */
    virtual double estimateAllotments(
        const InputData& input, const InputLinks& links, const MarketData& market) const override;
};

}   // namespace sea
