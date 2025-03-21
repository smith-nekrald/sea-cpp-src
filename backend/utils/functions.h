/**
 * @file functions.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Commonly used functions in backends and outside.
 * @copyright (c) Smith School of Business, 2025
 */
#pragma once

#include "../../input/input_data.h"
#include "../../input/input_links.h"
#include "../../algorithm/state.h"

namespace sea {
namespace backend {

/**
 * @brief Computes the capacity that remains available for a particular arc.
 *
 * @param input The input data.
 * @param links The input links.
 * @param state The state of current evaluation.
 * @param relativeTime Relative time at the consideration.
 * @param idxARc The index of the arc for computing available capacity.
 * @return The computed available arc capacity.
 *
 */
double computeAvailableArcCapacity(
        const InputData& input,
        const InputLinks& links,
        const State& state,
        unsigned relativeTime,
        unsigned idxArc);

/**
 * @brief Computes the capacity bottleneck for itinerary.
 *
 * @param input The input data.
 * @param links The input links.
 * @param state The state of current evaluation.
 * @param relativeTime Relative time at the consideration.
 * @param idxRoute The index of the route for computing bottleneck capacity.
 * @return The computed itinerary capacity bottleneck.
 *
 */
double computeItineraryBottleneck(
        const InputData& input,
        const InputLinks& links,
        const State& state,
        unsigned relativeTime,
        unsigned idxRoute);

/**
 * @brief Computs shipping cost per TEU for a given itinerary.
 *
 * @param input Input data, contains statistical input information.
 * @param links Input links, contains pre-computed structures based on input data.
 * @param idxRoute The index of the itinerary.
 * @return double The value of unit shipping cost.
 */
double computeUnitShippingCost(
        const InputData& input, const InputLinks& links, size_t idxRoute);


} // namespace backend
} // namespace sea

