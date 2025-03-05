#pragma once

#include "baseline_stats.h"
#include "../../input/input_data.h"
#include "../../input/input_links.h"


namespace sea {
namespace backend {
namespace spot {

struct ItineraryPlan {
public:
    size_t itineraryId;
    double price;
    double demand;
    double expectedRevenue;
};

ItineraryPlan buildItineraryPlan(
        const InputData& input,
        const InputLinks& links,
        const BaselineStats& stats,
        const InputData::Itinerary& route,
        const Demand& demand);

double computeRevenueProxy(
        double price, double amount, double shippingCost, double returnPrice, double showProba);

}
}
}
