// Implements commonly used in backends and outside functions.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2025

#include "functions.h"

#include <limits>
#include <set>
#include <cassert>
#include <cmath>

namespace sea {
namespace backend {

double computeItineraryBottleneck(
        const InputData& input,
        const InputLinks& links,
        const State& state,
        unsigned relativeTime,
        unsigned idxRoute) {
    double bottleneckCapacity = std::numeric_limits<double>::max();
    if (relativeTime == std::numeric_limits<unsigned>::max()) {
        relativeTime = 0;
    }
    const auto& route = input.itineraries[idxRoute];
    for (unsigned idxArc: route.orderedArcs) {
        double availableCapacity = computeAvailableArcCapacity(
            input, links, state, relativeTime, idxArc);
        bottleneckCapacity = std::min(availableCapacity, bottleneckCapacity);
    }
    return bottleneckCapacity;
}

double computeAvailableArcCapacity(
        const InputData& input,
        const InputLinks& links,
        const State& state,
        unsigned relativeTime,
        unsigned idxArc) {
    double availableCapacity = std::numeric_limits<double>::max();
    const auto& arc = input.arcs[idxArc];
    if (arc.type == InputData::Arc::Type::travel) {
        unsigned idxVessel = arc.vesselId.value();
        const auto& vessel = input.vessels[idxVessel];
        availableCapacity = vessel.capacity;
        std::set<unsigned> consideredRoutes;
        for (unsigned idxThrough : links.itinerariesWithArc[idxArc]) {
            assert(consideredRoutes.find(idxThrough) == consideredRoutes.end());
            const auto& routeThrough = input.itineraries[idxThrough];
            unsigned routeCutOff = links.itineraryIdToRelativeCutoffTime[idxThrough];
            bool itineraryShipped = (routeCutOff < relativeTime);
            if (itineraryShipped) {
                availableCapacity -= state.carriedOnRoute[idxThrough];
            } else {
                availableCapacity -= state.accumulatedBookings[idxThrough] *
                    routeThrough.showRate.estimatedProba;
                availableCapacity -= state.expectedAllotmentCapacity[idxThrough];
            }
            consideredRoutes.insert(idxThrough);
        }
        availableCapacity = std::max(0., availableCapacity);
    }
    return availableCapacity;
}

double computeUnitShippingCost(
        const InputData& input, const InputLinks& links,
        size_t idxRoute, bool countHiringOffhiring) {
    const InputData::Itinerary& route = input.itineraries[idxRoute];
    double shippingCost = route.cost;

    size_t idxStartArc = route.orderedArcs.front();
    const InputData::Arc& startArc = input.arcs[idxStartArc];
    const InputData::Node& startNode = input.nodes[startArc.fromNode];
    const InputData::Port& startPort = input.ports[startNode.portId];
    if (countHiringOffhiring) {
        shippingCost += startPort.hiringCost;
    }
    shippingCost += links.itineraryIdToCutoffDuration[idxRoute] * startPort.storageCost;

    size_t idxFinalArc = route.orderedArcs.back();
    const InputData::Arc& finalArc = input.arcs[idxFinalArc];
    const InputData::Node& finalNode = input.nodes[finalArc.toNode];
    const InputData::Port& finalPort = input.ports[finalNode.portId];
    if (countHiringOffhiring) {
        shippingCost += finalPort.offHiringCost;
    }

    return shippingCost;
}

double computeSpotRevenueProxy(
        double price, double amount, double shippingCost, double returnPrice, double showProba) {
    return amount * (
            showProba * (price - shippingCost) + (1. - showProba) * (price - returnPrice));
}

double getDemandByPrice(const Demand& demand, double price) {
    assert(price >= 0.);
    if (demand.type == Demand::Type::linear) {
        return std::max(0., demand.additive + demand.multiplicative * price);
    } else if (demand.type == Demand::Type::exponential) {
        return demand.scale * std::exp(-price * demand.sensitivity);
    } else {
        throw std::domain_error("Unexpected demand type.");
    }
}

} // namespace backend
} // namespace sea
