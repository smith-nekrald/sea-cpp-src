#include "plan.h"
#include "api.h"

#include <limits>
#include <cmath>
#include <cassert>

namespace sea {
namespace backend {
namespace spot {

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
    double optimalDemand = 0.;
    double optimalPrice = INF;
    const double EPS = 1e-3;
    if (demand.type == Demand::Type::exponential) {
        optimalDemand = demand.scale / std::exp(
            route.showRate.estimatedProba * (1 + shippingCost * demand.sensitivity)
            + (1 - route.showRate.estimatedProba) * (1 + route.returnPrice * demand.sensitivity));
        optimalDemand = std::min(optimalDemand, maxCapacity);
        const double LOG_EPS = 1e-50;
        optimalPrice = 1. / demand.sensitivity * std::log(
                LOG_EPS + demand.scale / optimalDemand);
        assert(optimalDemand <= demand.scale + EPS);
    } else if (demand.type == Demand::Type::linear) {
        assert(demand.multiplicative < 0.);
        optimalPrice = 0.5 * (
            shippingCost * route.showRate.estimatedProba
            + route.returnPrice * (1 - route.showRate.estimatedProba)
            - demand.additive / demand.multiplicative);
        optimalPrice = std::max(optimalPrice,
            (demand.additive - maxCapacity) / (-demand.multiplicative));
        optimalPrice = std::min(optimalPrice, demand.additive / (-demand.multiplicative));
        optimalDemand = demand.additive + optimalPrice * demand.multiplicative;
        assert(optimalDemand + EPS >= 0. && optimalDemand <= demand.additive + EPS);
    } else {
        throw std::logic_error("Unexpected demand type!");
    }
    // Robustness in prices and demand.
    optimalPrice += EPS;
    optimalDemand += EPS;
    assert(optimalPrice >= 0);

    // Revenue-based verification.
    double expectedRevenue
        = route.showRate.estimatedProba * optimalDemand * (optimalPrice - shippingCost)
        + (1 - route.showRate.estimatedProba) * optimalDemand * (optimalPrice - route.returnPrice);
    if (expectedRevenue + EPS < 0.) {
        optimalPrice = INF;
        optimalDemand = EPS;
        expectedRevenue = 0.;
    }
    return {idxRoute, optimalPrice, optimalDemand, expectedRevenue};
}


} // namespace spot
} // namespace backend
} // namespace sea
