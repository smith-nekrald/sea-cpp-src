#include "optimization_problem.h"
#include "../../logging/logging.h"

#include <cppad/ipopt/solve.hpp>

namespace sea {
namespace backend {

using CppAD::AD;

// Optimization Problem Logic.
OptimizationProblem::OptimizationProblem(
        const OptimizationConfig& aConfig)
    : config(aConfig) {
}


void OptimizationProblem::operator()(ADvector& functions, const ADvector& variables) {
    auto& logger = logging::getBackendSubLogger(BackendType::IPOPT);
    auto debugStream = logger.getStream(log4cpp::Priority::DEBUG);

    debugStream << "Entered OptimizationProblem::operator()\n";

    const double SCALE = config.scale;

    debugStream << "SCALE = " << SCALE << "\n";

    auto& input = config.inputManager->getConstData();
    auto& links = config.linksManager->getConstData();
    auto& indexMap = config.indexManager->getConstData();

    functions.assign(indexMap.constraintCount + 1, 0.0);
    debugStream << "Initialized functions with 0.\n";

    debugStream << "Variables count: " << variables.size() << "\n";
    debugStream << "Functions count: " << indexMap.constraintCount + 1 << "\n";
    debugStream << "Constraint count: " << indexMap.constraintCount << "\n";

    // State variables.
    vector<AD<double>> bookings(input.itineraries.size(), 0.);
    debugStream  << "Initialized bookings with 0.\n";

    vector<AD<double>> takens(input.itineraries.size(), 0.);
    debugStream << "Initialized takens with 0.\n";

    vector<AD<double>> containers(input.ports.size(), 0.);
    debugStream << "Initialized containers with 0.\n";

    for (ui32 portId = 0; portId < input.ports.size(); ++portId) {
        containers[portId] = input.ports[portId].initialContainerCount / SCALE;
        debugStream << "Setting containers[portId] to value: "
            << containers[portId] << " for port id = " << portId << "\n";
    }

    // Objective.

    AD<double>& objective = functions[0];
    debugStream << "Initializing objective as reference to functions[0]";


    for (ui32 timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        debugStream << "Considering time = " << timeNow << "\n";
        const InputData::Event& event = input.events[timeNow];
        double timeDelta = 0.0;
        if (timeNow + 1 < input.events.size()) {
            const InputData::Event& nextEvent = input.events[timeNow + 1];
            timeDelta = nextEvent.realTime - event.realTime;
        }
        debugStream << "Computed timeDelta as " << timeDelta << "\n";

        // Event-related recalculation.
        if (event.type == InputData::Event::Type::pricing) {
            debugStream << "Considering pricing event." << "\n";
            debugStream << "Will now iterate over event.relatedItineraryIds.\n";
            for (ui32 relatedIndex = 0; relatedIndex < event.relatedItineraryIds.size(); ++relatedIndex) {

                ui32 idItinerary = event.relatedItineraryIds[relatedIndex];
                debugStream << "Related idItinerary is " << idItinerary << "\n";

                ui32 demandIndex = indexMap.timeItineraryToDemandIndex[timeNow][idItinerary];
                debugStream << "Calculated demandIndex from indexMap as " << demandIndex << "\n";

                const AD<double>& demandVar = variables[demandIndex];
                debugStream << "Taking demandVar as variables[demandIndex]" << "\n";

                const auto& demand = event.demands[relatedIndex];
                debugStream << "Getting demand by event.demands as event.demands[relatedIndex]" << "\n";

                AD<double> revenue = 0.0;
                debugStream << "Creating AD<double> revenue.\n";

                debugStream << "If case." << "\n";
                debugStream << "timeNow = " << timeNow << "\n";
                debugStream << "config.updateTime = " << config.updateTime << "\n";
                if (timeNow < config.updateTime) {
                    debugStream << "If case. Entered branch where timeNow < config.updateTime" << "\n";
                    auto* decision = &config.decisionManager->getConstData();
                    revenue = decision->prices[timeNow][relatedIndex].second * demandVar * SCALE;
                    debugStream << "Revenue is calculated as decision->prices[timeNow][relatedIndex].second * demandVar * SCALE" << "\n";
                    debugStream <<  "set revenue to " << decision->prices[timeNow][relatedIndex].second * demandVar * SCALE << "\n";
                } else {
                    debugStream << "If case. Entred branch where timeNow >= config.updateTime" << "\n";
                    debugStream << "Switch case. Selecting between linear and exponential demands." << "\n";
                    if (demand.type == Demand::Type::linear) {
                        debugStream << "Demand is linear." << "\n";
                        revenue = SCALE * demandVar * (SCALE * demandVar - demand.additive) / demand.multiplicative;
                        debugStream << "Revenue is calculated as SCALE * demandVar * (SCALE * demandVar - demand.additive) / demand.multiplicative" << "\n";
                        debugStream << "demand.additive = " << demand.additive << "\n";
                        debugStream << "demand.multiplicative = " << demand.multiplicative << "\n";
                    } else if (demand.type == Demand::Type::exponential) {
                        debugStream << "Demand is exponential." << "\n";
                        assert(demand.scale > 0);
                        revenue = SCALE * demandVar * (- 1.0 / demand.sensitivity * log(1e-40 + demandVar * SCALE / demand.scale));
                        debugStream << "Revenue is calculated as SCALE * demandVar * (- 1.0 / demand.sensitivity * log(demandVar * SCALE / demand.scale))" << "\n";
                        debugStream << "demand.sensitivity = " << demand.sensitivity << "\n";
                        debugStream << "demand.scale = " << demand.scale << "\n";
                    } else {
                        throw std::logic_error("Demand is of strange type!");
                    }
                }
                debugStream << "Adding demandVar to bookings.\n";
                bookings[idItinerary] += demandVar;

                debugStream << "Adding revenue to objective.\n";
                objective += revenue;
            }
        } else {
            debugStream << "Considering non-pricing event.\n";

            ui32 idBasedArc = event.basedArc.value();
            const auto& arc = input.arcs[idBasedArc];

            debugStream << "Computed based arc.\n";

            if (event.type == InputData::Event::Type::cutoff) {

                debugStream << "Considering cut-off event.\n";

                ui32 idNode = arc.fromNode;
                debugStream << "Arc.fromNode = " << idNode << "\n";

                const auto& node = input.nodes[idNode];

                ui32 idPort = node.portId;
                debugStream << "Node.portid = " << idPort << "\n";

                const auto& port = input.ports[idPort];

                ui32 hireIndex = indexMap.arcToHired[idBasedArc];

                debugStream << "hireIndex is computed as indexMap.arcToHired[idBasedArc]" << "\n";
                debugStream << "hireIndex is set to " << hireIndex << "\n";

                const AD<double>& hired = variables[hireIndex];
                debugStream << "AD<double>& hired = variables[hireIndex]" << "\n";

                containers[idPort] += hired;
                debugStream << "Adding to containers[idPort] amount of hired containers.\n";

                objective -= port.hiringCost * hired * SCALE;
                debugStream << "Subtracting port.hiringCost * hired * SCALE from objective.\n";

                debugStream << "For loop over links.itinerariesFromArc[idBasedArc] \n";
                for (ui32 idItinerary : links.itinerariesFromArc[idBasedArc]) {
                    debugStream << "Considering itinerary with id = " << idItinerary << "\n";

                    const auto& itinerary = input.itineraries[idItinerary];
                    ui32 qSpotIndex = indexMap.idItineraryToQIndex[idItinerary];
                    debugStream << "Computing " << "ui32 qSpotIndex = indexMap.idItineraryToQIndex[idItinerary]" << "\n";
                    debugStream << "qSpotIndex = " << qSpotIndex << "\n";

                    ui32 zIndex = indexMap.idItineraryToZIndex[idItinerary];
                    debugStream << "zIndex = " << zIndex << "\n";

                    const AD<double>& qSpot = variables[qSpotIndex];
                    const AD<double>& zEmpty = variables[zIndex];
                    debugStream << "Assigning AD<double> to qSpot and zEmpty\n";

                    takens[idItinerary] += qSpot;
                    takens[idItinerary] += zEmpty;
                    debugStream << "Adding qSpot and zEmpty to takens[idItinerary]\n";

                    containers[idPort] -= qSpot;
                    containers[idPort] -= zEmpty;
                    debugStream << "Subtracting qSpot and zEmpty from containers[idPort]\n";

                    ui32 qIdConstraint = indexMap.spotQNConstraints[idItinerary];
                    debugStream << "Taking qIdConstraint as indexMap.spotQNConstraints[idItinerary]" << "\n";

                    auto& spotQNConstraint = functions[qIdConstraint];
                    debugStream << "Obtaining spotQNConstraint as reference in functions" << "\n";

                    AD<double> expectedShow = 0.;
                    debugStream << "Computing expectedShow. Intializing AD<double> expectedShow.\n";

                    debugStream << "If case: timeNow [< or >=]  config.updateTime" << "\n";
                    debugStream << "Value of timeNow: " << timeNow << "\n";
                    debugStream << "Value of config.updateTime: " << config.updateTime << "\n";

                    if (timeNow < config.updateTime) {
                        debugStream << "Entering branch timeNow < config.updateTime." << "\n";
                        auto* action = &config.actionManager->getConstData();
                        expectedShow = action->spotMarketDemandN[idItinerary] / SCALE;
                        debugStream << "Assigining to expectedShow value in action, divided by scale" << "\n";
                    } else {
                        debugStream << "Entering branch timeNow >= config.updateTime." << "\n";
                        expectedShow = bookings[idItinerary] * itinerary.showRate.estimatedProba;
                        debugStream << "expectedShow  = bookings[idItinerary] * itinerary.showRate.estimatedProba" << "\n";
                        debugStream << "itinerary.showRate.estimatedProba = " << itinerary.showRate.estimatedProba << "\n";
                    }

                    spotQNConstraint = expectedShow - qSpot; // inf >= spotQNConstraint >= 0
                    debugStream << "Assiging spotQNConstraint as show >= qSpot" << "\n";

                    objective -= SCALE * bookings[idItinerary] * itinerary.returnPrice;
                    debugStream << "subtracting itinerary.returnPrice from objective" << "\n";

                    objective += SCALE * expectedShow * (-itinerary.declineCost + itinerary.returnPrice);
                    debugStream << " adding diff return - decline" << "\n";

                    objective += SCALE * qSpot * (-itinerary.cost + itinerary.declineCost - event.duration * port.storageCost);
                    debugStream << "adding SCALE * qSpot" << "\n";

                    objective -= SCALE * zEmpty * itinerary.emptyCost;
                    debugStream << "removing SCALE * zEmpty * itinerary.emptyCost" << "\n";

                    debugStream << "running cycle by allotmentsWithItinerary for itinerary " << idItinerary <<  "\n";
                    for (ui32 idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                        debugStream << "idAllotment = " << idAllotment << "\n";

                        ui32 qAllotmentIndex = indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
                        debugStream << "qAllotmentIndex = " << qAllotmentIndex << "\n";
                        const AD<double>& qAllotment = variables[qAllotmentIndex];

                        ui32 uAllotmentIndex = indexMap.allotmentToUIndex[idAllotment];
                        debugStream << "uAllotmentIndex = " << uAllotmentIndex << "\n";
                        const AD<double>& uAllotment = variables[uAllotmentIndex];

                        if (!config.useEnhancedVersion) {
                            takens[idItinerary] += uAllotment * qAllotment;
                            containers[idPort] -= uAllotment * qAllotment;
                        } else {
                            takens[idItinerary] += qAllotment;
                            containers[idPort] -= qAllotment;

                        }

                        auto entryId = links.allotmentItineraryToEntry.at(idAllotment).at(idItinerary);
                        debugStream << "allotmentId = " << idAllotment << " idItinerary = "
                            << idItinerary << " idAllotmentEntry = " << entryId << "\n";
                        const auto& entry = input.allotmentEntries[entryId];

                        debugStream << "computing expectedAllotmentShow: " << "\n";
                        double expectedAllotmentShow = 0.;
                        debugStream << "initialized with zero." << "\n";
                        if (timeNow < config.updateTime) {
                            debugStream << "timeNow < config.updateTime ==> updating from action" << "\n";
                            ui32 place = links.allotmentItineraryToPlace.at(idAllotment).at(idItinerary);
                            auto* action = &config.actionManager->getConstData();
                            expectedAllotmentShow = action->allotmentDemandN[idAllotment][place].second / SCALE;
                            assert(action->allotmentDemandN[idAllotment][place].first == idItinerary);
                        } else {
                            debugStream << "timeNow >= config.updateTime ==> estimating from entry.showRate.estimatedProba" << "\n";
                            expectedAllotmentShow = entry.showRate.estimatedProba * entry.productAmount / SCALE;
                            debugStream << "entry.showRate.estimatedProba = " << entry.showRate.estimatedProba << "\n";
                            debugStream << "entry.productAmount = " << entry.productAmount << "\n";
                        }
                        debugStream << "scaled expectedAllotmentShow = " << expectedAllotmentShow << "\n";

                        objective += SCALE * uAllotment * expectedAllotmentShow * (-entry.cancellationPrice - itinerary.declineCost);
                        debugStream << "modified objective to expectedAllotmentShow";

                        if (!config.useEnhancedVersion) {
                            objective += uAllotment * qAllotment * (
                                    entry.price - itinerary.cost + itinerary.declineCost - event.duration * port.storageCost) * SCALE;
                        } else {
                            objective += qAllotment * (
                                    entry.price - itinerary.cost + itinerary.declineCost - event.duration * port.storageCost) * SCALE;
                        }
                        debugStream << "modified objective wrt qAllotment" << "\n";

                        double cancellationPrice = entry.productAmount * entry.cancellationPrice / SCALE;
                        objective += SCALE * uAllotment * cancellationPrice;
                        if (config.useEnhancedVersion) {
                            ui32 itineraryAllotmentQConstraintIndex = indexMap.allotmentItineraryQConstraints[idItinerary][idAllotment];
                            debugStream << "itineraryAllotmentQConstraintIndex = " << itineraryAllotmentQConstraintIndex << "\n";
                            auto& itineraryAllotmentQConstraint = functions[itineraryAllotmentQConstraintIndex];
                            itineraryAllotmentQConstraint = qAllotment - uAllotment * expectedAllotmentShow; // -inf <= itineraryAllotmentQConstraint <= 0
                        }
                    }
                }

                // Containers constraint on cutoff
                ui32 cutoffPortConstraintIndex = indexMap.portPositiveCutoffArcConstraints[idBasedArc];
                auto& cutoffPortConstraint = functions[cutoffPortConstraintIndex];
                cutoffPortConstraint = containers[idPort]; // inf >= cutoffPortConstraint >= 0

            } else if (event.type == InputData::Event::Type::arrival) {
                ui32 idNode = arc.toNode;
                const auto& node = input.nodes[idNode];
                const auto& port = input.ports[node.portId];
                const auto& offhired = variables[indexMap.timeToSIndex[event.relativeTime]];
                objective -= SCALE * offhired  * port.offHiringCost;
                containers[port.id] -= offhired;
                for (ui32 idItinerary : links.itinerariesToArc[idBasedArc]) {
                    containers[port.id] += takens[idItinerary];
                }
                // Containers constraint on arrival
                ui32 arrivalPortConstraintIndex = indexMap.portPositiveArrivalArcConstraints[idBasedArc];
                auto& arrivalPortConstraint = functions[arrivalPortConstraintIndex];
                arrivalPortConstraint = containers[port.id]; // inf >= arrivalPortConstraint >= 0
            }
        }
        // Common recalculation.
        for (ui32 idPort = 0; idPort < input.ports.size(); ++idPort) {
            const auto& port = input.ports[idPort];
            objective -= SCALE * (containers[idPort] * timeDelta) * port.storageCost;
        }
    }

    // Capacity constraints
    for (ui32 idArc = 0; idArc < input.arcs.size(); ++idArc) {
        const auto& arc = input.arcs[idArc];
        if (arc.type == InputData::Arc::Type::travel) {
            AD<double> inside_arc = 0.0;
            for (ui32 idItinerary : links.itinerariesWithArc[idArc]) {
                inside_arc += takens[idItinerary];
            }
            ui32 constraintIndex = indexMap.arcCapacityConstraints[idArc];
            AD<double>& constraint = functions[constraintIndex];
            constraint = inside_arc * SCALE - input.vessels[arc.vesselId.value()].capacity * config.utilizationRatio; // -inf <= constraint  <=0
        }
    }

    // Constraints on final container count
    for (ui32 idPort = 0; idPort < input.ports.size(); ++idPort) {
        double finalCount = input.ports[idPort].finalContainerCount;
        AD<double>& constraint = functions[indexMap.finalContainerConstraints[idPort]];
        constraint = finalCount / SCALE - containers[idPort]; // 0 >= constraint >= -INF
        objective -= (-finalCount/SCALE + containers[idPort]) * (input.ports[idPort].offHiringCost * SCALE);
    }

    // Group constraints
    for (ui32 idGroup = 0; idGroup < input.allotmentGroups.size(); ++idGroup) {
        const auto& group = input.allotmentGroups[idGroup];
        auto& constraint = functions[indexMap.groupConstraints[idGroup]];
        for (ui32 idAllotment : group) {
            const auto& uAllotment = variables[indexMap.allotmentToUIndex[idAllotment]];
            constraint += uAllotment;
        }
        // 0 <= constraint <= 1
    }
    objective = -objective;

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


