

namespace sea {
namespace backend {

double AbstractSpotMarketStrategy::computeExpectedUnitProfit(
        const InputData::Event& event, unsigned placeIdx) const {
    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();
    const auto& decision = decisionManager->getConstData();
    unsigned idxRoute = event.relatedItineraryIds[placeIdx];
    const auto& route = input.itineraries[idxRoute];
    double price = decision.prices[event.relativeTime][placeIdx].second;
    double demand = backend::getDemandByPrice(event.demands[placeIdx], price);
    double shippingCost = backend::computeUnitShippingCost(input, links, idxRoute, false);
    double returnPrice = route.returnPrice;
    return backend::computeSpotRevenueProxy(
            price, demand, shippingCost, returnPrice, route.showRate.estimatedProba);
}


std::vector<unsigned> AbstractSpotMarketStrategy::preparePricingPlaceIds(
        const InputData::Event& event) const {
    const auto& decision = decisionManager->getConstData();

    unsigned priceRouteCount = event.relatedItineraryIds.size();
    assert(priceRouteCount == decision.prices[event.relativeTime].size());
    assert(priceRouteCount == event.demands.size());

    std::vector<unsigned> placeIdsOrder(priceRouteCount);
    std::vector<double> expectedProfits(priceRouteCount);
    for (unsigned idxPlace = 0; idxPlace < priceRouteCount; ++idxPlace) {
        placeIdsOrder[idxPlace] = idxPlace;
        expectedProfits[idxPlace] = computeExpectedUnitProfit(event, idxPlace);
    }
    std::sort(placeIdsOrder.begin(), placeIdsOrder.end(),
            [&] (unsigned lhsPlace, unsigned rhsPlace) {
                return expectedProfits[lhsPlace] > expectedProfits[rhsPlace];
            });
    return placeIdsOrder;
}

void AbstractSpotMarketStrategy::correctPricing(const InputData::Event& event) {
    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();
    auto& decision = decisionManager->getData();
    std::map<unsigned, double> idxArcToAddedCapacity;

    std::vector<unsigned> placeIdsOrder = preparePricingPlaceIds(event);
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



} // namespace backend
} // namespace sea
