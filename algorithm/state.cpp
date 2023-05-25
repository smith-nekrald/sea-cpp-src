// Implements functions declared in state. 

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include <cassert>

#include "state.h"
#include "../manager.h"


namespace sea {

void initState(const InputData& input, State* state) {
    const double INF = std::numeric_limits<double>::max();
    assert(state != nullptr);
    state->accumulatedBookings.assign(input.itineraries.size(), 0);
    state->accumulatedBookings.shrink_to_fit();
    state->takenOnRoute.resize(input.itineraries.size(), 0);
    state->takenOnRoute.shrink_to_fit();
    state->itineraryFutureEstimation.assign(input.itineraries.size(), 0);
    state->itineraryFutureEstimation.shrink_to_fit();
    state->timeParameters = TimeParameters();
    state->containersInPorts.assign(input.ports.size(), 0);
    state->containersInPorts.shrink_to_fit();
    state->usedCapacity.assign(input.arcs.size(), 0);
    state->usedCapacity.shrink_to_fit();
    state->estimatedObjective = -INF;
}

void toNextEvent(TimeParameters* parameters) {
    parameters->gotPortDecision = false;
    parameters->doneDecision = false;
    parameters->doneAction = false;
    ++parameters->timeEvent;
}
} // namespace sea
