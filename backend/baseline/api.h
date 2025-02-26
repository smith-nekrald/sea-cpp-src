#pragma once
#include "baseline_stats.h"
#include "../../input/input_data.h"

namespace sea {
namespace backend {

void allocatePreciseCapacityForItinerary(
        BaselineStats* stats, const InputData& input, size_t idxRoute, size_t amount);

double computeExpectedAvailableCapacityForItinerary(
        const InputData& input, const BaselineStats& stats, size_t idxItinerary);

size_t computeExactAllocationCapacityForItinerary(
        const InputData& input, const BaselineStats& stats, size_t idxRoute);

bool checkIfAllotmentAvailable(
        const InputData& input, const BaselineStats& stats, size_t idxAllotment);

void updateStatsAtAllotmentSelection(
        BaselineStats* stats, const InputData& input, size_t idxAllotment);

} // namespace sea
} // namespace backend
