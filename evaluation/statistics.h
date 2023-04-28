#pragma once

#include "../types.h"
#include "estimator.h"

#include <fstream>
#include <iostream>


namespace sea {

struct Statistics {
    double spotProfit = 0.0;
    double allotmentProfit = 0.0;
    double containerProfit = 0.0;
    double emptyContainerProfit = 0.0;

    ui32 sumSpotBookingAmount = 0;
    ui32 sumAllotmentBookingAmount = 0;

    ui32 emptyContainerCount = 0;
    ui32 spotContainerCount = 0;
    ui32 allotmentContainerCount = 0;

    double fullProfit = 0.0; // = spotProfit + allotmentProfit + containerProfit + emptyContainerProfit
    RevenueEstimate estimation = {0, 0};
};

void printEvaluatorStatistics(const Statistics& stats, std::ostream& out);

}   // namespace sea
