#include "plan.h"
#include "api.h"

#include <limits>
#include <cmath>
#include <cassert>
#include <ranges>

namespace sea {
namespace backend {
namespace spot {


double computeRevenueProxy(
        double price, double amount, double shippingCost, double returnPrice, double showProba) {
    return amount * (
            showProba * (price - shippingCost) + (1. - showProba) * (price - returnPrice));
}

ItineraryPlan buildItineraryPlan(
        const InputData& input,
        const InputLinks& links,
        const BaselineStats& stats,
        const InputData::Itinerary& route,
        const Demand& demand) {
    const double INF = std::numeric_limits<double>::max();
    size_t idxRoute = route.id;
    // Find maximal available capacity.
    double freeCapacity
        = backend::computeExpectedAvailableCapacityForItinerary(input, stats, idxRoute);
    double maxCapacity = freeCapacity / route.showRate.estimatedProba;
    const double ONE_TEU = 1.;
    if (maxCapacity < ONE_TEU) {
        return {idxRoute, INF, 0, 0};
    }
    // Compute complete shipping cost.
    double shippingCost = backend::computeUnitShippingCost(input, links, idxRoute);

    // Find optimal price and demand.
    double optimalDemand = 0., optimalPrice = INF;
    std::vector<double> borderPrices, borderDemands;
    const double DIVIDE_EPS = 1e-100;
    const double COMPARE_EPS = 1e-3;
    if (demand.type == Demand::Type::exponential) {
        const double LOG_EPS = 1e-300;
        double refundDemand = demand.scale * std::exp(-demand.sensitivity * route.returnPrice);
        double shippingDemand = demand.scale * std::exp(-demand.sensitivity * shippingCost);
        double maxDemand = std::min({demand.scale, refundDemand, shippingDemand, maxCapacity});
        double priceAtMax = std::max(0., 1. / (demand.sensitivity + DIVIDE_EPS)
            * std::log(LOG_EPS + demand.scale / (maxDemand + DIVIDE_EPS)));
        priceAtMax = std::max({priceAtMax, route.returnPrice, shippingCost});
        double minDemand = 0.;
        double priceAtMin = INF;
        assert(minDemand <= maxDemand);

        borderPrices = {priceAtMin, priceAtMax};
        borderDemands = {minDemand, maxDemand};

        optimalDemand = demand.scale / std::exp(
            route.showRate.estimatedProba * (1 + shippingCost * demand.sensitivity)
            + (1 - route.showRate.estimatedProba) * (1 + route.returnPrice * demand.sensitivity));
        optimalDemand = std::min(maxDemand, optimalDemand);
        optimalDemand = std::max(optimalDemand, minDemand);
        assert(optimalDemand <= demand.scale + COMPARE_EPS);
        optimalPrice = std::max(0., 1. / (demand.sensitivity + DIVIDE_EPS) * std::log(
                LOG_EPS + demand.scale / (optimalDemand + DIVIDE_EPS)));

    } else if (demand.type == Demand::Type::linear) {
        assert(demand.multiplicative < 0.);
        optimalPrice = 0.5 * (
            shippingCost * route.showRate.estimatedProba
            + route.returnPrice * (1 - route.showRate.estimatedProba)
            - demand.additive / (-DIVIDE_EPS + demand.multiplicative));
        double capacityPrice = (demand.additive - maxCapacity) / (-demand.multiplicative);
        double zeroDemandPrice = demand.additive / (-demand.multiplicative);

        double minPrice = std::max({route.returnPrice, capacityPrice, 0., shippingCost});
        double demandAtMin = std::max(0., demand.additive + minPrice * demand.multiplicative);

        double maxPrice = std::max(zeroDemandPrice, minPrice);
        double demandAtMax = std::max(0., demand.additive + maxPrice * demand.multiplicative);
        assert (minPrice <= maxPrice);

        borderPrices = {minPrice, maxPrice};
        borderDemands = {demandAtMin, demandAtMax};

        optimalPrice = std::min(optimalPrice, maxPrice);
        optimalPrice = std::max(optimalPrice, minPrice);
        optimalDemand = std::max(0., demand.additive + optimalPrice * demand.multiplicative);

        assert(optimalDemand + COMPARE_EPS >= 0.
                && optimalDemand <= demand.additive + COMPARE_EPS);
    } else {
        throw std::logic_error("Unexpected demand type!");
    }
    double optimalRevenue = computeRevenueProxy(
            optimalPrice, optimalDemand, shippingCost,
            route.returnPrice, route.showRate.estimatedProba);
    for (auto [price, amount] : std::views::zip(borderPrices, borderDemands)) {
        double revenue = computeRevenueProxy(
                price, amount, shippingCost, route.returnPrice, route.showRate.estimatedProba);
        if (revenue > optimalRevenue) {
            std::tie(optimalPrice, optimalDemand, optimalRevenue) = {price, amount, revenue};
        }
    }

    assert(optimalPrice >= 0 && optimalRevenue >= 0 and optimalDemand >= 0);
    assert(optimalPrice + COMPARE_EPS >= shippingCost
            && optimalPrice + COMPARE_EPS >= route.returnPrice);
    return {idxRoute, optimalPrice, optimalDemand, optimalRevenue};
}


} // namespace spot
} // namespace backend
} // namespace sea

