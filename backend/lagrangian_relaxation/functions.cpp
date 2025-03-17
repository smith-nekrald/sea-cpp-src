// Commonly used functions.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
// (c) Smith School of Business, 2025

#include "functions.h"
#include "../../input/input_data.h"

#include <limits>
#include <set>

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
        const auto& arc = input.arcs[idxArc];
        if (arc.type == InputData::Arc::Type::travel) {
            unsigned idxVessel = arc.vesselId.value();
            const auto& vessel = input.vessels[idxVessel];
            double availableCapacity = vessel.capacity;
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
                }
                consideredRoutes.insert(idxThrough);
            }
            availableCapacity = std::max(0., availableCapacity);
            bottleneckCapacity = std::min(availableCapacity, bottleneckCapacity);
        }
    }
    return bottleneckCapacity;
}

double vectorAbsDiffSum(const std::vector<double>& lhs, const std::vector<double>& rhs) {
    assert(lhs.size() == rhs.size());
    double result = 0;
    for (unsigned index = 0; index < lhs.size(); ++index) {
        result += fabsl(lhs[index] - rhs[index]);
    }
    return result;
}

double dualAbsDiffSum(const DualVariables& lhs, const DualVariables& rhs) {
    return vectorAbsDiffSum(lhs.lambdaVariables, rhs.lambdaVariables) +
        vectorAbsDiffSum(lhs.muVariables, rhs.muVariables);
}

double dualL2Norm(const DualVariables& duals) {
    double norm = 0;
    for (const auto& item : duals.lambdaVariables) {
        norm += item * item;
    }
    for (const auto& item : duals.muVariables) {
        norm += item * item;
    }
    return sqrt(norm);
}

double dualL1Norm(const DualVariables& duals) {
    double norm = 0;
    for (const auto& item : duals.lambdaVariables) {
        norm += fabsl(item);
    }
    for (const auto& item : duals.muVariables) {
        norm += fabsl(item);
    }
    return norm;
}

unsigned countNonZeroElements(
        const SubgradientOptimizationParameters& parameters, const double EPS) {
    unsigned result = 0;
    for (unsigned idLambda = 0; idLambda < parameters.lambdaSubgradient.size(); ++idLambda) {
        if (fabs(parameters.lambdaSubgradient[idLambda]) > EPS) {
            ++result;
        }
    }
    for (unsigned idMu = 0; idMu < parameters.muSubgradient.size(); ++idMu) {
        if (fabs(parameters.muSubgradient[idMu]) > EPS) {
            ++result;
        }
    }
    return result;
}


} // namespace backend
} // namespace sea
