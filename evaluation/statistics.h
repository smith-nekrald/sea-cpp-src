/**
 * @file statistics.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines Statistics - an entity with evaluation summary on a particular trajectory.
 * Also defines a method to print Statistics into stream.
 * @copyright (c) Smith School of Business, 2023
 * @copyright (c) Smith School of Business, 2025
 */
#pragma once

#include "estimator.h"

#include <iostream>


namespace sea {

/**
 * @brief Structure to store evaluation summary on a particular trajectory.
 */
struct Statistics {
    /// @brief Spot market profit.
    double spotProfit = 0.0;
    /// @brief Allotment profit.
    double allotmentProfit = 0.0;
    /// @brief Profit from container management (negative).
    double containerProfit = 0.0;
    /// @brief Profit from emtpy containers (negative).
    double emptyContainerProfit = 0.0;

    /// @brief The number of spot demand bookings.
    unsigned sumSpotBookingAmount = 0;
    /// @brief The number of allotment demand bookings.
    unsigned sumAllotmentBookingAmount = 0;

    /// @brief The number of times when empty container was transported over some itinerary.
    unsigned emptyContainerCount = 0;
    /// @brief The number of times a spot container was transported over some itinerary.
    unsigned spotContainerCount = 0;
    /// @brief The number of times an allotment TEU was transported over some itinerary.
    unsigned allotmentContainerCount = 0;

    /// @brief The number of TEU shippings declined at spot market.
    unsigned declinedAtSpot = 0;
    /// @brief The number of TEU shippings declined at allotment market.
    unsigned declinedAtAllotment = 0;
    /// @brief The number of TEU shippings declined in total.
    unsigned declinedInTotal = 0;

    /// @brief The cost of declines at the spot market.
    double declinePaidSpot = 0.;
    /// @brief The cost of declines at the allotment market.
    double declinePaidAllotment = 0.;
    /// @brief The cost of declines in total.
    double declinePaidTotal = 0.;

    /// @brief Profit over the trajectory.
    /// fullProfit = spotProfit + allotmentProfit + containerProfit + emptyContainerProfit
    double fullProfit = 0.0;

    /// @brief Estimation for the profit upper bound.
    RevenueEstimate estimation = {0, 0};
};

/**
 * @brief Prints statistics into a stream.
 *
 * @param[in] stats Statistics to print.
 * @param[out] out Stream to write the statistics into.
 */
void printEvaluatorStatistics(const Statistics& stats, std::ostream& out);

}   // namespace sea
