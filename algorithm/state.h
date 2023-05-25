#pragma once

#include <vector>

#include "../types.h"
#include "../input/input_data.h"

namespace sea {

/**
 * @brief Represents the timing on the current trajectory.
 * Stores the next event time, whether allotments are supplied,
 * and whether the corresponding action and decision are processed.
 */
struct TimeParameters {
    /// @brief If port decision is received.
    bool gotPortDecision = false;

    /// @brief Boolean flag if decision is received and processed.
    bool doneDecision = false;

    /// @brief Boolean flag if action is received and processed.
    bool doneAction = false;

    /// @brief Boolean flag if allotments are already supplied.
    bool allotmentsSupplied = false;

    /// @brief Index of the considered time event.
    unsigned int timeEvent = 0;
};

void toNextEvent(TimeParameters* parameters);

/**
 * @brief State represents
 *
 */
struct State {
    std::vector<unsigned int> accumulatedBookings; // b_r
    std::vector<unsigned int> takenOnRoute; // n_{t,r} + l_{t,r}
    std::vector<int> containersInPorts;
    TimeParameters timeParameters;
    std::vector<unsigned int> usedCapacity; // indexed by arcs
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
