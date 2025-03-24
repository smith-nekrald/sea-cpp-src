// Implementation and API related to Statistics structure.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "statistics.h"

#include <iostream>
#include <cassert>
#include <cmath>

namespace sea {

using std::ostream;

void printEvaluatorStatistics(const Statistics& stats, ostream& os) {

    os << std::fixed;
    os << stats.estimation;
    os << "complete_profit = " << std::fixed <<  stats.fullProfit << std::endl;
    os << std::fixed;
    os << "full_profit = " << stats.fullProfit
        << " spot_market_profit = " << stats.spotProfit
        << " allotment_profit = " << stats.allotmentProfit
        << " container_profit (should be negative) = " << stats.containerProfit
        << " empty_container_profit (should be negative) = " << stats.emptyContainerProfit
        << " empty_containers_sum = " << stats.emptyContainerCount
        << " spot_market_containers = " << stats.spotContainerCount
        << " allotment_market_containers = " << stats.allotmentContainerCount
        << " bookings_spot = " << stats.sumSpotBookingAmount
        << " bookings_allotment = " << stats.sumAllotmentBookingAmount
        << " declined_spot = " << stats.declinedAtSpot
        << " declined_allotment = " << stats.declinedAtAllotment
        << " declined_total = " << stats.declinedInTotal
        << std::endl;

    const double EPS = 1e-2;
    assert(fabs(stats.fullProfit - (stats.spotProfit + stats.allotmentProfit +
            stats.containerProfit + stats.emptyContainerProfit)) < EPS);
}

} // namespace sea
