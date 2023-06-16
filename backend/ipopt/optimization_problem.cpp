// Optimization Problem Logic.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "optimization_problem.h"
#include "../../logging/logging.h"

#include <cppad/ipopt/solve.hpp>

namespace sea {
namespace backend {

using CppAD::AD;

OptimizationProblem::OptimizationProblem(const OptimizationConfig& aConfig) : config(aConfig) {}

void OptimizationProblem::processPricing(const unsigned timeNow,
                                        const InputData::Event& event,
                                        const vector<AD<double>>& variables,
                                        vector<AD<double>>* functionsPtr,
                                        vector<AD<double>>* bookingsPtr) const {

    auto& indexMap = config.indexManager->getConstData();
    vector<AD<double>>& functions = *functionsPtr;
    AD<double>& objective = functions[0];
    vector<AD<double>>& bookings = *bookingsPtr;
    assert(event.type == InputData::Event::Type::pricing);
    for (unsigned relatedIndex = 0; relatedIndex < event.relatedItineraryIds.size();
            ++relatedIndex) {

        unsigned idItinerary = event.relatedItineraryIds[relatedIndex];
        unsigned demandIndex = indexMap.timeItineraryToDemandIndex[timeNow][idItinerary];

        const AD<double>& demandVar = variables[demandIndex];
        const auto& demand = event.demands[relatedIndex];
        AD<double> revenue = 0.0;

        if (timeNow < config.updateTime) {
            auto* decision = &config.decisionManager->getConstData();
            revenue = decision->prices[timeNow][relatedIndex].second * demandVar;
        } else {
            if (demand.type == Demand::Type::linear) {
                revenue = demandVar * (demandVar - demand.additive) / demand.multiplicative;
            } else if (demand.type == Demand::Type::exponential) {
                assert(demand.scale > 0);
                const double LOG_EPS = 1e-40;
                revenue = demandVar * ( -1.0 / demand.sensitivity
                    * log(LOG_EPS + demandVar / demand.scale));
            } else {
                throw std::logic_error("Demand is of strange type!");
            }
        }
        bookings[idItinerary] += demandVar;
        objective += revenue;
    }
}

void OptimizationProblem::processCutoff(const unsigned timeNow,
                                        const InputData::Event& event,
                                        const vector<AD<double>>& variables,
                                        vector<AD<double>>* functionsPtr,
                                        vector<AD<double>>* bookingsPtr,
                                        vector<AD<double>>* boardedPtr,
                                        vector<AD<double>>* containersPtr) const {

    auto& input = config.inputManager->getConstData();
    auto& links = config.linksManager->getConstData();
    auto& indexMap = config.indexManager->getConstData();

    vector<AD<double>>& bookings = *bookingsPtr;
    vector<AD<double>>& boarded = *boardedPtr;
    vector<AD<double>>& containers = *containersPtr;

    vector<AD<double>>& functions = *functionsPtr;
    AD<double>& objective = functions[0];

    unsigned idBasedArc = event.basedArc.value();
    const auto& arc = input.arcs[idBasedArc];

    assert(event.type == InputData::Event::Type::cutoff);

    unsigned idNode = arc.fromNode;
    const auto& node = input.nodes[idNode];

    unsigned idPort = node.portId;
    const auto& port = input.ports[idPort];
    unsigned hireIndex = indexMap.arcToHired[idBasedArc];

    const AD<double>& hired = variables[hireIndex];

    containers[idPort] += hired;
    objective -= port.hiringCost * hired;

    for (unsigned idItinerary : links.itinerariesFromArc[idBasedArc]) {

        const auto& itinerary = input.itineraries[idItinerary];
        unsigned qSpotIndex = indexMap.idItineraryToQIndex[idItinerary];

        unsigned zIndex = indexMap.idItineraryToZIndex[idItinerary];

        const AD<double>& qSpot = variables[qSpotIndex];
        const AD<double>& zEmpty = variables[zIndex];

        boarded[idItinerary] += qSpot;
        boarded[idItinerary] += zEmpty;

        containers[idPort] -= qSpot;
        containers[idPort] -= zEmpty;

        unsigned qIdConstraint = indexMap.spotQNConstraints[idItinerary];
        auto& spotQNConstraint = functions[qIdConstraint];
        AD<double> expectedShow = 0.;

        if (timeNow < config.updateTime) {
            auto* action = &config.actionManager->getConstData();
            expectedShow = action->spotMarketDemandN[idItinerary];
        } else {
            expectedShow = bookings[idItinerary] * itinerary.showRate.estimatedProba;
        }

        spotQNConstraint = expectedShow - qSpot; // inf >= spotQNConstraint >= 0
        objective -= bookings[idItinerary] * itinerary.returnPrice;
        objective += expectedShow * (-itinerary.declineCost + itinerary.returnPrice);
        objective += qSpot * (-itinerary.cost + itinerary.declineCost
            - event.duration * port.storageCost);
        objective -= zEmpty * itinerary.emptyCost;
        for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {

            unsigned qAllotmentIndex
                = indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
            const AD<double>& qAllotment = variables[qAllotmentIndex];

            unsigned uAllotmentIndex = indexMap.allotmentToUIndex[idAllotment];
            const AD<double>& uAllotment = variables[uAllotmentIndex];

            boarded[idItinerary] += qAllotment;
            containers[idPort] -= qAllotment;

            auto entryId = links.allotmentItineraryToEntry.at(idAllotment).at(idItinerary);
            const auto& entry = input.allotmentEntries[entryId];

            double expectedAllotmentShow = 0.;
            if (timeNow < config.updateTime) {
                unsigned place = links.allotmentItineraryToPlace.at(idAllotment).at(idItinerary);
                auto* action = &config.actionManager->getConstData();
                expectedAllotmentShow = action->allotmentDemandN[idAllotment][place].second;
                assert(action->allotmentDemandN[idAllotment][place].first == idItinerary);
            } else {
                expectedAllotmentShow = entry.showRate.estimatedProba * entry.productAmount;
            }

            objective += uAllotment * expectedAllotmentShow * (
                -entry.cancellationPrice - itinerary.declineCost);

            objective += qAllotment * (entry.price - itinerary.cost
                + itinerary.declineCost - event.duration * port.storageCost);

            double cancellationPrice = entry.productAmount * entry.cancellationPrice;
            objective += uAllotment * cancellationPrice;
            unsigned itineraryAllotmentQConstraintIndex
                = indexMap.allotmentItineraryQConstraints[idItinerary][idAllotment];
            auto& itineraryAllotmentQConstraint = functions[itineraryAllotmentQConstraintIndex];

            // -inf <= itineraryAllotmentQConstraint <= 0
            itineraryAllotmentQConstraint = qAllotment - uAllotment * expectedAllotmentShow;
        }
    }

    // Containers constraint on cutoff
    unsigned cutoffPortConstraintIndex = indexMap.portPositiveCutoffArcConstraints[idBasedArc];
    auto& cutoffPortConstraint = functions[cutoffPortConstraintIndex];
    cutoffPortConstraint = containers[idPort]; // inf >= cutoffPortConstraint >= 0
}

void OptimizationProblem::processArrival(const InputData::Event& event,
                                        const vector<AD<double>>& variables,
                                        vector<AD<double>>* functionsPtr,
                                        vector<AD<double>>* boardedPtr,
                                        vector<AD<double>>* containersPtr) const {

    auto& input = config.inputManager->getConstData();
    auto& links = config.linksManager->getConstData();
    auto& indexMap = config.indexManager->getConstData();

    vector<AD<double>>& boarded = *boardedPtr;
    vector<AD<double>>& containers = *containersPtr;

    vector<AD<double>>& functions = *functionsPtr;
    AD<double>& objective = functions[0];

    unsigned idBasedArc = event.basedArc.value();
    const auto& arc = input.arcs[idBasedArc];

    assert(event.type == InputData::Event::Type::arrival);
    unsigned idNode = arc.toNode;
    const auto& node = input.nodes[idNode];
    const auto& port = input.ports[node.portId];
    const auto& offhired = variables[indexMap.timeToSIndex[event.relativeTime]];
    objective -= offhired  * port.offHiringCost;
    containers[port.id] -= offhired;
    for (unsigned idItinerary : links.itinerariesToArc[idBasedArc]) {
        containers[port.id] += boarded[idItinerary];
    }
    // Containers constraint on arrival
    unsigned arrivalPortConstraintIndex = indexMap.portPositiveArrivalArcConstraints[idBasedArc];
    auto& arrivalPortConstraint = functions[arrivalPortConstraintIndex];
    arrivalPortConstraint = containers[port.id]; // inf >= arrivalPortConstraint >= 0
}

void OptimizationProblem::formCapacityConstraints(
        const vector<CppAD::AD<double>>& boarded,
        vector<CppAD::AD<double>>* functionsPtr) const {

    auto& input = config.inputManager->getConstData();
    auto& links = config.linksManager->getConstData();
    auto& indexMap = config.indexManager->getConstData();

    vector<CppAD::AD<double>>& functions = *functionsPtr;

    // Capacity constraints
    for (unsigned idArc = 0; idArc < input.arcs.size(); ++idArc) {
        const auto& arc = input.arcs[idArc];
        if (arc.type == InputData::Arc::Type::travel) {
            AD<double> inside_arc = 0.0;
            for (unsigned idItinerary : links.itinerariesWithArc[idArc]) {
                inside_arc += boarded[idItinerary];
            }
            unsigned constraintIndex = indexMap.arcCapacityConstraints[idArc];
            AD<double>& constraint = functions[constraintIndex];
            constraint = inside_arc - input.vessels[arc.vesselId.value()].capacity
                * config.utilizationRatio; // -inf <= constraint  <= 0
        }
    }
}

void OptimizationProblem::formFinalContainerConstraints(
        const vector<CppAD::AD<double>>& containers,
        vector<CppAD::AD<double>>* functionsPtr) const {

    auto& input = config.inputManager->getConstData();
    auto& indexMap = config.indexManager->getConstData();

    vector<CppAD::AD<double>>& functions = *functionsPtr;
    AD<double>& objective = functions[0];

    // Constraints on final container count
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        double finalCount = input.ports[idPort].finalContainerCount;
        AD<double>& constraint = functions[indexMap.finalContainerConstraints[idPort]];
        constraint = finalCount - containers[idPort]; // 0 >= constraint >= -INF
        objective -= (-finalCount + containers[idPort]) * (input.ports[idPort].offHiringCost);
    }
}

void OptimizationProblem::formGroupConstraints(
        const vector<CppAD::AD<double>>& variables,
        vector<CppAD::AD<double>>* functionsPtr) const {

    auto& input = config.inputManager->getConstData();
    auto& indexMap = config.indexManager->getConstData();

    vector<CppAD::AD<double>>& functions = *functionsPtr;
    // Group constraints
    for (unsigned idGroup = 0; idGroup < input.allotmentGroups.size(); ++idGroup) {
        const auto& group = input.allotmentGroups[idGroup];
        auto& constraint = functions[indexMap.groupConstraints[idGroup]];
        for (unsigned idAllotment : group) {
            const auto& uAllotment = variables[indexMap.allotmentToUIndex[idAllotment]];
            constraint += uAllotment;
        }
        // 0 <= constraint <= 1
    }
}

void OptimizationProblem::operator()(ADvector& functions, const ADvector& variables) {

    auto& input = config.inputManager->getConstData();
    auto& indexMap = config.indexManager->getConstData();

    functions.assign(indexMap.constraintCount + 1, 0.0);

    // State variables.
    vector<AD<double>> bookings(input.itineraries.size(), 0.);
    vector<AD<double>> boarded(input.itineraries.size(), 0.);
    vector<AD<double>> containers(input.ports.size(), 0.);

    // Initial container counts.
    for (unsigned portId = 0; portId < input.ports.size(); ++portId) {
        containers[portId] = input.ports[portId].initialContainerCount;
    }

    // Objective and event-related constraints.
    AD<double>& objective = functions[0];
    for (unsigned timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        const InputData::Event& event = input.events[timeNow];
        double timeDelta = 0.0;
        if (timeNow + 1 < input.events.size()) {
            const InputData::Event& nextEvent = input.events[timeNow + 1];
            timeDelta = nextEvent.realTime - event.realTime;
        }

        // Event-related recalculation.
        if (event.type == InputData::Event::Type::pricing) {
            processPricing(timeNow, event, variables, &functions, &bookings);
        } else if (event.type == InputData::Event::Type::cutoff) {
            processCutoff(timeNow, event, variables, &functions, &bookings, &boarded, &containers);
        } else if (event.type == InputData::Event::Type::arrival) {
            processArrival(event, variables, &functions, &boarded, &containers);
        }

        // Common recalculation.
        for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
            const auto& port = input.ports[idPort];
            objective -= (containers[idPort] * timeDelta) * port.storageCost;
        }
    }

    // Forming the rest of constraints and updating objective when needed.
    formCapacityConstraints(boarded, &functions);
    formFinalContainerConstraints(containers, &functions);
    formGroupConstraints(variables, &functions);

    // Modifying objective sign for right optimization direction.
    objective = -objective;

    // Releasing managers in case memory efficient version is required.
    releaseManagers();
}

void OptimizationProblem::releaseManagers() {

    if (config.actionManager != nullptr) {
        config.actionManager->release();
    }
    config.linksManager->release();
    config.inputManager->release();
    if (config.decisionManager != nullptr) {
        config.decisionManager->release();
    }
    config.indexManager->release();
}

} // namespace backend
} // namespace sea
