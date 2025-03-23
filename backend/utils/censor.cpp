#include "censor.h"
#include "functions.h"

#include <cmath>
#include <vector>
#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <map>

namespace sea {
namespace backend {

SpotPricingCensor::SpotPricingCensor(ConstInputManagerPtr initInputManager,
                                     ConstLinksManagerPtr initLinksManager)
        : inputManager(initInputManager)
        , linksManager(initLinksManager) {
}

void SpotPricingCensor::correctPricing(
        const InputData::Event& event,
        const State& state,
        DecisionManagerPtr decisionManager) {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    auto& decision = decisionManager->getData();
    std::map<unsigned, double> idxArcToAddedCapacity;

    std::vector<unsigned> placeIdsOrder = preparePricingPlaceIds(event, decision);
    for (unsigned idxPlace: placeIdsOrder) {
        unsigned idxRoute = event.relatedItineraryIds[idxPlace];
        const auto& route = input.itineraries[idxRoute];

        double shippingCost = backend::computeUnitShippingCost(input, links, idxRoute, false);
        assert(decision.prices[event.relativeTime][idxPlace].first == idxRoute);
        double correctedPrice = std::max({
                decision.prices[event.relativeTime][idxPlace].second,
                route.returnPrice, shippingCost, 0.
        });
        double correctedDemand = 0.;

        double capacityBottleneck = std::numeric_limits<double>::max();
        for (unsigned idxArc : route.orderedArcs) {
            const auto& arc = input.arcs[idxArc];
            if (arc.type == InputData::Arc::Type::travel) {
                double priorCapacity = backend::computeAvailableArcCapacity(
                    input, links, state, event.relativeTime, idxArc);
                double availableCapacity = std::max(
                        0., priorCapacity - idxArcToAddedCapacity[idxArc]);
                capacityBottleneck = std::min(availableCapacity, capacityBottleneck);
            }
        }
        double bottleneckDemand = capacityBottleneck / route.showRate.estimatedProba;

        const auto& demand = event.demands[idxPlace];
        if (demand.type == Demand::Type::linear) {
            correctedPrice = std::max(correctedPrice,
                    (demand.additive - bottleneckDemand) / (-demand.multiplicative));
            correctedDemand = std::max(
                    0., demand.additive + correctedPrice * demand.multiplicative);
        } else if (demand.type == Demand::Type::exponential) {
            const double LOG_EPS = 1e-100;
            correctedPrice = std::max(correctedPrice,
                (1. / demand.sensitivity * std::log((demand.scale + LOG_EPS)
                                            / (bottleneckDemand + LOG_EPS))));
            correctedDemand = demand.scale * std::exp(-demand.sensitivity * correctedPrice);
        } else {
            throw std::domain_error("Unexpected demand type.");
        }
        double correctedCapacity = correctedDemand * route.showRate.estimatedProba;
        decision.prices[event.relativeTime][idxPlace].second = correctedPrice;
        for (unsigned idxArc : route.orderedArcs) {
            idxArcToAddedCapacity[idxArc] += correctedCapacity;
        }
    }
}

double SpotPricingCensor::computeExpectedUnitProfit(
        const InputData::Event& event,
        const Decision& decision,
        unsigned placeIdx) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    unsigned idxRoute = event.relatedItineraryIds[placeIdx];
    const auto& route = input.itineraries[idxRoute];
    double price = decision.prices[event.relativeTime][placeIdx].second;
    double demand = backend::getDemandByPrice(event.demands[placeIdx], price);
    double shippingCost = backend::computeUnitShippingCost(input, links, idxRoute, false);
    double returnPrice = route.returnPrice;
    return backend::computeSpotRevenueProxy(
            price, demand, shippingCost, returnPrice, route.showRate.estimatedProba);
}

std::vector<unsigned> SpotPricingCensor::preparePricingPlaceIds(
        const InputData::Event& event,
        const Decision& decision) const {

    unsigned priceRouteCount = event.relatedItineraryIds.size();
    assert(priceRouteCount == decision.prices[event.relativeTime].size());
    assert(priceRouteCount == event.demands.size());

    std::vector<unsigned> placeIdsOrder(priceRouteCount);
    std::vector<double> expectedProfits(priceRouteCount);
    for (unsigned idxPlace = 0; idxPlace < priceRouteCount; ++idxPlace) {
        placeIdsOrder[idxPlace] = idxPlace;
        expectedProfits[idxPlace] = computeExpectedUnitProfit(event, decision, idxPlace);
    }
    std::sort(placeIdsOrder.begin(), placeIdsOrder.end(),
            [&] (unsigned lhsPlace, unsigned rhsPlace) {
                return expectedProfits[lhsPlace] > expectedProfits[rhsPlace];
            });
    return placeIdsOrder;
}

} // namespace backend
} // namespace sea

