/**
 * @file plan.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines ItineraryPlan and API for computing.
 * @copyright (c) Smith School of Business, 2025
 */
#pragma once

#include "baseline_stats.h"
#include "../../input/input_data.h"
#include "../../input/input_links.h"


namespace sea {
namespace backend {
namespace spot {

/**
 * @brief Itinerary plan for a spot
 * market pricing on a fixed route.
 */
struct ItineraryPlan {
public:
    ///@brief Id of the considered itinerary.
    size_t itineraryId;
    ///@brief Price to set for the considered itinerary.
    double price;
    /// @brief Demand based on the suggested price for the considered itinerary.
    double demand;
    /// @brief Expected revenue for the suggested price on the considered itinerary.
    double expectedRevenue;
};

/**
 * @brief Builds an itinerary plan for a spot market pricing on a fixed route.
 * The itinerary plan suggests optimal price, demand and expected revenue.
 *
 * @param input The input data, configuration and statistical information.
 * @param links The input links, precomputed structures and info on top of input data.
 * @param stats The baseline stats, current state summary.
 * @param route The considered route to process.
 * @param demand The demand for the considered route.
 * @return The itinerary plan with suggested price, demand and expected revenue.
 */
ItineraryPlan buildItineraryPlan(
        const InputData& input,
        const InputLinks& links,
        const BaselineStats& stats,
        const InputData::Itinerary& route,
        const Demand& demand);

} // namespace spot
} // namespace backend
} // namespace sea
