/**
 * @file api.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares commonly used functions for the baseline algorithm.
 * @copyright (c) Smith School of Business, 2025
 */
#pragma once

#include "baseline_stats.h"
#include "../../input/input_data.h"
#include "../../input/input_links.h"

namespace sea {
namespace backend {

/**
 * @brief Allocates precise integer capacity for a given itinerary.
 *
 * @param stats The baseline stats to be updated.
 * @param input Input data, contains statistical input information.
 * @param idxRoute The index of the itinerary.
 * @param amount Amount of capacity to be allocated.
 */
void allocatePreciseCapacityForItinerary(
        BaselineStats* stats, const InputData& input, size_t idxRoute, size_t amount);

/**
 * @brief Computes the expected spot market allocation
 * capacity available for a given itinerary.
 *
 * @param input Input data, contains statistical input information.
 * @param stats The current baseline stats.
 * @param idxItinerary The index of the itinerary.
 * @return double Expected capacity available for allocation on the itinerary.
 */
double computeExpectedAvailableCapacityForItinerary(
        const InputData& input, const BaselineStats& stats, size_t idxItinerary);

/**
 * @brief Computes the exact integer allocation available capacity for a given itinerary.
 *
 * @param input Input data, contains statistical input information.
 * @param stats The current baseline stats.
 * @param idxRoute The index of the itinerary.
 * @return size_t Capacity available for allocation on the itinerary.
 */
size_t computeExactAllocationCapacityForItinerary(
        const InputData& input, const BaselineStats& stats, size_t idxRoute);

/**
 * @brief Checks if the allotment can be selected given the current baseline stats.
 *
 * @param input Input data, contains statistical input information.
 * @param stats The current baseline stats.
 * @param idxAllotment The index of the allotment to be selected.
 * @return True if allotment can be selected without violating expected capacity and
 *        allotment groups, False otherwise.
 */
bool checkIfAllotmentAvailable(
        const InputData& input, const BaselineStats& stats, size_t idxAllotment);

/**
 * @brief Updates baseline stats at the allotment selection.
 *
 * @param stats The baseline stats to be updated.
 * @param input Input data, contains statistical input information.
 * @param idxAllotment The index of the selected allotment.
 */
void updateStatsAtAllotmentSelection(
        BaselineStats* stats, const InputData& input, size_t idxAllotment);

/**
 * @brief Computes expected allotment profit on selection assuming future shipping.
 *
 * @param input Input data, contains statistical input information.
 * @param links Input links, contains pre-computed structures based on input data.
 * @param allotmentId The index of the allotment.
 * @return double The expected profit from shipping allotment.
 */

double computeExpectedAllotmentProfit(
        const InputData& input, const InputLinks& links, unsigned allotmentId);

} // namespace sea
} // namespace backend
