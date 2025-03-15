// Implements commonly used functions in the baseline algorithm.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2025

#include "api.h"

#include <cassert>
#include <limits>
#include <map>
#include <set>

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
    std::map<unsigned, double> arcToRequiredCapacity;
    std::set<unsigned> involvedItineraries, involvedArcs;
    for (const auto& idxEntry: allotment.entries) {
        const auto& entry = input.allotmentEntries[idxEntry];
        size_t idxItinerary = entry.itinerary;
        assert(involvedItineraries.find(idxItinerary) == involvedItineraries.end());
        involvedItineraries.insert(idxItinerary);
        double capacityConsumption = entry.productAmount * entry.showRate.estimatedProba;
        const auto& route = input.itineraries[idxItinerary];
        for (unsigned idxArc: route.orderedArcs) {
            involvedArcs.insert(idxArc);
            arcToRequiredCapacity[idxArc] += capacityConsumption;
        }
    }
    for (unsigned idxArc: involvedArcs) {
        const InputData::Arc& arc = input.arcs[idxArc];
        if (arc.type == InputData::Arc::Type::travel) {
            double requiredCapacity = arcToRequiredCapacity[idxArc];
            double availableCapacity = stats.availableArcCapacity[idxArc];
            const double EPS = 1e-3;
            if (availableCapacity < requiredCapacity + EPS) {
                return false;
            }
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
    for (unsigned idxEntry: allotment.entries) {
        const auto& entry = input.allotmentEntries[idxEntry];
        double expectedCapacity = entry.productAmount * entry.showRate.estimatedProba;
        stats->allocatedLongEntryCapacity[entry.id] += expectedCapacity;
        const InputData::Itinerary& route = input.itineraries[entry.itinerary];
        for (unsigned idxArc : route.orderedArcs) {
            const InputData::Arc& arc = input.arcs[idxArc];
            if (arc.type == InputData::Arc::Type::travel) {
                [[maybe_unused]] const double EPS = 1e-5;
                assert(stats->availableArcCapacity[arc.id] >= EPS + expectedCapacity);
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

double computeExpectedAllotmentProfit(
    const InputData& input, const InputLinks& links, unsigned allotmentId) {
    const auto& allotment = input.allotments[allotmentId];
    double allotmentExpectedProfit = 0.;
    for (unsigned idxEntry = 0; idxEntry < allotment.entries.size(); ++idxEntry) {
        const auto& entry = input.allotmentEntries[allotment.entries[idxEntry]];
        double shippingCost = computeUnitShippingCost(input, links, entry.itinerary);
        allotmentExpectedProfit += entry.productAmount * (
            (entry.price - shippingCost) * entry.showRate.estimatedProba +
            entry.cancellationPrice * (1. - entry.showRate.estimatedProba)
        );
    }
    return allotmentExpectedProfit;
}


} // namespace sea
} // namespace backend
