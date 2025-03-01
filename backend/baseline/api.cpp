#include "api.h"

#include <cassert>
#include <limits>

namespace sea {
namespace backend {

void allocatePreciseCapacityForItinerary(
        BaselineStats* stats, const InputData& input, size_t idxRoute, size_t amount) {
    const auto& route = input.itineraries[idxRoute];
    for (size_t idArc: route.orderedArcs) {
        assert(stats->freeArcCapacity[idArc] >= amount);
        stats->freeArcCapacity[idArc] -= amount;
    }
}

double computeExpectedAvailableCapacityForItinerary(
        const InputData& input, const BaselineStats& stats, size_t idxItinerary) {
    const auto& itinerary = input.itineraries[idxItinerary];
    double minimalCapacity = std::numeric_limits<double>::max();
    for (size_t idArc: itinerary.orderedArcs) {
        minimalCapacity = std::min<double>(
                stats.availableArcCapacity[idArc], minimalCapacity);
    }
    const double ONE_TEU = 1.;
    if (minimalCapacity < ONE_TEU) {
        minimalCapacity = 0.;
    }
    return minimalCapacity;
}

size_t computeExactAllocationCapacityForItinerary(
        const InputData& input, const BaselineStats& stats, size_t idxRoute) {
    const auto& route = input.itineraries[idxRoute];
    size_t freeCapacity = std::numeric_limits<size_t>::max();
    for (size_t idArc: route.orderedArcs) {
        freeCapacity = std::min<size_t>(stats.freeArcCapacity[idArc], freeCapacity);
    }
    return freeCapacity;
}

bool checkIfAllotmentAvailable(
        const InputData& input, const BaselineStats& stats, size_t idxAllotment) {
    const auto& allotment = input.allotments[idxAllotment];
    for (size_t groupIdx = 0; groupIdx < input.allotmentGroups.size(); ++groupIdx) {
        const std::vector<unsigned>& group = input.allotmentGroups[groupIdx];
        for (unsigned allotmentIndex: group) {
            if (allotmentIndex == allotment.id && stats.ifSelectedFromGroup[groupIdx]) {
                return false;
            }
        }
    }
    for (const auto& entry: input.allotmentEntries) {
        size_t idxItinerary = entry.itinerary;
        double availableCapacity = backend::computeExpectedAvailableCapacityForItinerary(
                input, stats, idxItinerary);
        if (availableCapacity < entry.productAmount * entry.showRate.estimatedProba) {
            return false;
        }
    }
    return true;
}

void updateStatsAtAllotmentSelection(
        BaselineStats* stats, const InputData& input, size_t idxAllotment) {
    const auto& allotment = input.allotments[idxAllotment];
    for (size_t idxGroup = 0; idxGroup < input.allotmentGroups.size(); ++idxGroup) {
        for (size_t idxAllotmentInGroup: input.allotmentGroups[idxGroup]) {
            if (idxAllotmentInGroup == allotment.id) {
                stats->ifSelectedFromGroup[idxGroup] = true;
            }
        }
    }
    for (const auto& entry: input.allotmentEntries) {
        const InputData::Itinerary& route = input.itineraries[entry.itinerary];
        double expectedCapacity = entry.productAmount * entry.showRate.estimatedProba;
        stats->allocatedLongEntryCapacity[entry.id] += expectedCapacity;
        for (unsigned idxArc : route.orderedArcs) {
            const InputData::Arc& arc = input.arcs[idxArc];
            if (arc.type == InputData::Arc::Type::travel) {
                stats->availableArcCapacity[arc.id] -= expectedCapacity;
            }
        }
    }
}

double computeUnitShippingCost(
        const InputData& input, const InputLinks& links, size_t idxRoute) {
    const InputData::Itinerary& route = input.itineraries[idxRoute];
    double shippingCost = route.cost;

    size_t idxStartArc = route.orderedArcs.front();
    const InputData::Arc& startArc = input.arcs[idxStartArc];
    const InputData::Node& startNode = input.nodes[startArc.fromNode];
    const InputData::Port& startPort = input.ports[startNode.portId];
    shippingCost += startPort.hiringCost;
    shippingCost += links.itineraryIdToCutoffDuration[idxRoute] * startPort.storageCost;

    size_t idxFinalArc = route.orderedArcs.back();
    const InputData::Arc& finalArc = input.arcs[idxFinalArc];
    const InputData::Node& finalNode = input.nodes[finalArc.toNode];
    const InputData::Port& finalPort = input.ports[finalNode.portId];
    shippingCost += finalPort.offHiringCost;

    return shippingCost;
}


} // namespace sea
} // namespace backend
