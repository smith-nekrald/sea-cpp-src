/**
 * @file state.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief State is an entity to store summary about the trajectory prefix in the algorithm.
 * @copyright (c) Smith School of Business, 2023
 */
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
    unsigned timeEvent = 0;
};

/**
 * @brief Updates TimeParameters to the beginning of the next time period.
 * This means doneDecison = doneAction = gotPortDecision = false, timeEvent += 1.
 * 
 * @param parameters The TimeParameters to update.
 */
void toNextEvent(TimeParameters* parameters);

/**
 * @brief State is an entity to stare parameters of the environment. E.g. amount booked,
 * amounts shipping, capacity, and containers in ports.
 */
struct State {
    /// @brief Amount of accumulated bookings, per itinerary. Denoted b_r in the model.
    std::vector<unsigned> accumulatedBookings;
    /// @brief Amount of taken and shipped TEU. Denoted n_{t,r} and l_{t,r} in the model.
    std::vector<unsigned> takenOnRoute; 
    /// @brief Amount of TEU currently stored in ports.
    std::vector<int> containersInPorts;
    /// @brief Time parameters structure to track current event.
    TimeParameters timeParameters;
    /// @brief Capacity utilization. Indexed by arcs.
    std::vector<unsigned> usedCapacity; 
    /// @brief Estimated objective, up to the end of time horizon.
    double estimatedObjective;
    /// @brief Estimated decomposed objective, indexed by itinerary. Denoted v_t^r in the model.
    std::vector<double> itineraryFutureEstimation; 
};

/**
 * @brief State initialization.
 * 
 * @param[in] input The InputData statistical informatino.
 * @param[out] state State to initialize.
 */
void initState(const InputData& input, State* state);

/**
 * @brief Prints time parameters into stream.
 * 
 * @tparam Writer Type of the stream.
 * @param[in] timeParameters  Parameters to export.
 * @param[out] writer Stream to write into.
 */
template<typename Writer>
void printTimeParameters(const TimeParameters& timeParameters, Writer& writer) {
    writer << "\nTimeParameters:\n";
    writer << "timeEvent = " << timeParameters.timeEvent << " ";
    writer << "doneDecision = " << timeParameters.doneDecision << " ";
    writer << "doneAction = " << timeParameters.doneAction << " ";
    writer << "allotmentsSupplied = " << timeParameters.allotmentsSupplied << " ";
    writer << "gotPortDecision = " << timeParameters.gotPortDecision << " ";
}
/**
 * @brief Prints state into stream.
 * 
 * @tparam Writer Type of the stream.
 * @param[in] state State to export.
 * @param[out] writer Stream to write into.
 */
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
