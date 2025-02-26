#include "baseline_stats.h"

#include <limits>
#include <cmath>

namespace sea {
namespace backend {

void initBaselineStats(BaselineStats* stats, const InputData& input) {
    const double FLOAT_INF = std::numeric_limits<double>::max();
    stats->availableArcCapacity.assign(input.arcs.size(), FLOAT_INF);
    const size_t LONG_INF = std::numeric_limits<size_t>::max();
    stats->freeArcCapacity.assign(input.arcs.size(), LONG_INF);
    stats->allocatedSpotRouteCapacity.assign(input.itineraries.size(), 0);
    stats->allocatedLongEntryCapacity.assign(input.allotmentEntries.size(), 0);
    stats->ifSelectedFromGroup.assign(input.allotmentGroups.size(), false);
    for (const auto& arc: input.arcs) {
        if (arc.type == InputData::Arc::Type::travel) {
            auto& vessel = input.vessels[arc.vesselId.value()];
            stats->availableArcCapacity[arc.id] = vessel.capacity;
            stats->freeArcCapacity[arc.id] = static_cast<size_t>(std::floor(vessel.capacity));
        }
    }
}


} // namespace backend
} // namespace sea
