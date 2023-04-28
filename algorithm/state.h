#pragma once

#include <vector>

#include "../types.h"
#include "../input/input_data.h"

namespace sea {

struct TimeParameters {
    bool gotPortDecision = false,
         doneDecision = false,
         doneAction = false,
         allotmentsSupplied = false;
    ui32 timeEvent = 0;
};

void toNextEvent(TimeParameters* parameters);

struct State {
    std::vector<ui32> accumulatedBookings; // b_r
    std::vector<ui32> takenOnRoute; // n_{t,r} + l_{t,r}
    std::vector<int> containersInPorts;
    TimeParameters timeParameters;
    std::vector<ui32> usedCapacity; // indexed by arcs
    double estimatedObjective; // up to the end

    std::vector<double> itineraryFutureEstimation; // v_t^r
};

void initState(const InputData& input, State* state);

template<typename Writer>
void printTimeParameters(const TimeParameters& timeParameters, Writer& writer) {
    writer << "\nTimeParameters:\n";
    writer << "timeEvent = " << timeParameters.timeEvent << " ";
    writer << "doneDecision = " << timeParameters.doneDecision << " ";
    writer << "doneAction = " << timeParameters.doneAction << " ";
    writer << "allotmentsSupplied = " << timeParameters.allotmentsSupplied << " ";
    writer << "gotPortDecision = " << timeParameters.gotPortDecision << " ";
}

template<typename Writer>
void printState(const State& state, Writer& writer) {
    writer << "\nState Information:";
    writer << "\naccumulatedBookings: \n";
    for (const auto& item : state.accumulatedBookings) {
        writer << item << " ";
    }
    writer << "\ntakenOnRoute: \n";
    for (const auto& item : state.takenOnRoute) {
        writer << item << " ";
    }
    writer << "\ncontainersInPorts: \n";
    for (const auto& item : state.containersInPorts) {
        writer << item << " ";
    }
    printTimeParameters(state.timeParameters, writer);
    writer << "\nusedCapacity: \n";
    for (const auto& item : state.usedCapacity) {
        writer << item << " ";
    }
    writer << "\nestimatedObjective: " <<  state.estimatedObjective;
    writer << "\nitineraryFutureEstimation:\n";
    for (const auto& item : state.itineraryFutureEstimation) {
        writer << item << " ";
    }
}

} // namespace sea
