/**
 * @file baseline_stats.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines Baseline Stats and initialization API.
 * @copyright (c) Smith School of Business, 2025 
 */
#pragma once

#include "../../input/input_data.h"

#include <vector>

namespace sea {
namespace backend {

/**
 * @brief Specific to Baseline set of structures and state.
 */
struct BaselineStats {
    /// @brief Approximation on the available capacity per arc. This approximation is relevant
    /// for making greedy decisions and tracking approximate capacity utilization.
    std::vector<double> availableArcCapacity;
    /// @brief For tracking real capacity allocations at cutoff events. This tracking allows
    /// to avoid allocating more than arc capacity.
    std::vector<size_t> freeArcCapacity;
    /// @brief Stores the amount of greedily allocated capacity per route.
    std::vector<double> allocatedSpotRouteCapacity;
    /// @brief Stores the amount of greedily allocated capacity per allotment entry.
    std::vector<double> allocatedLongEntryCapacity;
    /// @brief Tracks if each select-one group has already some allotment selected.
    std::vector<bool> ifSelectedFromGroup;
};

/**
 * @brief Initializes the baseline stats using the input data.
 * 
 * @param stats The baselines stats to initialize.
 * @param input The input data. Containts configuration and statistical information.
 */
void initBaselineStats(BaselineStats* stats, const InputData& input);


} // namespace backend
} // namespace sea
