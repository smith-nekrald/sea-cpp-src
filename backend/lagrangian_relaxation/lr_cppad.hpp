#pragma once

#include "lagrangian_relaxation_backend.h"
#include "index.h"
#include "lr_cppad.h"

#include "../../logging/logging.h"

#include <stdexcept>
#include <cppad/cppad.hpp>
#include <cppad/core/value.hpp>
#include <cppad/core/var2par.hpp>

namespace sea {
namespace backend {

using ArcType=InputData::Arc::Type;
using EventType=InputData::Event::Type;
using CppAD::AD;

double Value(const double& entry);
double Value(const CppAD::AD<double>& entry);


template <typename Type>
inline Type computeFunctionValue(const InputData& input,
                   const InputLinks& links,
                   const State& state,
                   const LagrangianRelaxationIndex& magicIndex,
                   const TimeParameters& timeParameters,
                   Decision* decision,
                   const DualTemplate<Type>& point,
                   bool ignoreSpot = false,
                   double l2RegConstant = 0,
                   DualVariables mean = DualVariables()){
    Type functionValue = Type(0);
    const double EPS = 1e-7;
    if (l2RegConstant > EPS) {
        Type regularizer = Type(0);
        for (size_t idx : {0, 1}) {
            auto& item = (idx == 0 ? point.lambdaVariables : point.muVariables);
            if (mean.lambdaVariables.size() || mean.muVariables.size()) {
                auto& midPoint = (idx == 0 ? mean.lambdaVariables : mean.muVariables);
                for (size_t ind = 0; ind < midPoint.size(); ++ind) {
                    regularizer += (item[ind] - midPoint[ind]) * (item[ind] - midPoint[ind]);
                }
            } else {
                for (const auto& entry : item) {
                    regularizer += entry * entry;
                }
            }
        }
        functionValue += (regularizer * l2RegConstant);
    }
    // Adding lambda * W_b to objective.
    Type scaledCapacitySum = 0.;
    const auto& theNextEvent = input.events[timeParameters.timeEvent];
    for (ui32 idArc = 0; idArc < input.arcs.size(); ++idArc) {
        const auto& arc = input.arcs[idArc];
        Type arcFromStartTime = input.nodes[arc.fromNode].realTime;
        if (arc.type == ArcType::travel && arcFromStartTime >= theNextEvent.realTime) {
            const auto& vessel = input.vessels[arc.vesselId.value()];
            ui32 place = magicIndex.arcToLambdaIndex.at(idArc);
            Type scaledCapacity = vessel.capacity * point.lambdaVariables[place];
            scaledCapacitySum += scaledCapacity;
        }
    }
    functionValue += scaledCapacitySum;
    for (const auto& event: input.events) {
        if (event.type == EventType::cutoff) {
            // Scenario on events that have already passed.
            if (event.relativeTime < timeParameters.timeEvent) {
                for (ui32 itineraryId : event.relatedItineraryIds) {
                    const auto& itinerary = input.itineraries[itineraryId];
                    for (ui32 idArc : itinerary.orderedArcs) {
                        const auto& arc = input.arcs[idArc];
                        if (arc.type == ArcType::travel) {
                            const auto& arcNode = input.nodes[arc.fromNode];
                            double timeArc = arcNode.realTime;
                            if (timeArc >= theNextEvent.realTime) {
                                ui32 place = magicIndex.arcToLambdaIndex.at(idArc);
                                Type taken = state.takenOnRoute[itinerary.id];
                                functionValue -= taken * point.lambdaVariables[place];
                            }
                        }
                    }
                }
            // Scenario for cutoffs that have not happened yet.
            } else {
                const auto& basedArc = input.arcs[event.basedArc.value()];
                for (ui32 itineraryId : event.relatedItineraryIds) {
                    const auto& itinerary = input.itineraries[itineraryId];
                    ui32 F_r = decision->nonEmptyContainersQ[itinerary.id] + decision->emptyContainersZ[itinerary.id];
                    Type lambdaSum = 0.;
                    for (ui32 idArc : itinerary.orderedArcs) {
                        const auto& arc = input.arcs[idArc];
                        if (arc.type == ArcType::travel) {
                            lambdaSum += point.lambdaVariables[magicIndex.arcToLambdaIndex.at(idArc)];
                        }
                    }
                    Type qr = 0, zr = 0;
                    qr -= lambdaSum;
                    zr -= lambdaSum;
                    qr -= itinerary.cost;
                    zr -= itinerary.emptyCost;
                    qr += itinerary.declineCost;
                    qr -= event.duration * input.ports[
                        input.nodes[basedArc.fromNode].portId].storageCost;
                    zr -= event.duration * input.ports[
                        input.nodes[basedArc.fromNode].portId].storageCost;
                    Type mu_r = point.muVariables[itinerary.id];

                    Type plusDiff = std::max(Type(0.), Type(qr - mu_r));
                    Type yr = -itinerary.returnPrice + itinerary.showRate.estimatedProba * (
                                -itinerary.declineCost + itinerary.returnPrice + plusDiff);

                    functionValue += yr * state.accumulatedBookings[itinerary.id];

                    const auto& demandPlaces = links.demandTimesForItinerary[itinerary.id];
                    pair<ui32, ui32> targetPair = std::make_pair(
                            timeParameters.timeEvent, ui32(0));
                    const auto& placeBegin = std::lower_bound(
                            demandPlaces.begin(), demandPlaces.end(), targetPair);

                    for (auto iter = placeBegin; iter != demandPlaces.end(); ++iter) {
                        const auto& pricingEvent = input.events[iter->first];
                        assert(pricingEvent.type == EventType::pricing);
                        assert(pricingEvent.relatedItineraryIds[iter->second] == itinerary.id);
                        const auto& demand = input.events[iter->first].demands[iter->second];
                        Type optimalPrice = 0., demandValue = 0.;
                        if (demand.type == Demand::Type::linear) {
                            assert(demand.additive >= 0 && demand.multiplicative <= 0);
                            Type leftPrice = 0.,
                                 rightPrice = -demand.additive / demand.multiplicative;

                            optimalPrice = std::max<Type>(leftPrice,
                                    -(demand.additive + demand.multiplicative * yr) /
                                    (2. * demand.multiplicative));
                            optimalPrice = std::min<Type>(rightPrice, optimalPrice);

                            if ((optimalPrice + yr) *
                                (demand.additive + demand.multiplicative * optimalPrice)
                                    <  (leftPrice + yr) *
                                    (demand.additive + leftPrice * demand.multiplicative)) {
                                optimalPrice = leftPrice;
                            }
                            if ((optimalPrice + yr) *
                                (demand.additive + demand.multiplicative * optimalPrice)
                                    <  (rightPrice + yr) *
                                    (demand.additive + rightPrice * demand.multiplicative)) {
                                optimalPrice = rightPrice;
                            }
                            demandValue = demand.additive
                                + optimalPrice * demand.multiplicative;
                        } else if (demand.type == Demand::Type::exponential) {
                            assert(demand.sensitivity >= 0 && demand.scale >= 0);
                            optimalPrice = std::max(Type(0.), 1. / demand.sensitivity - yr);
                            demandValue = demand.scale * exp(
                                    -demand.sensitivity * optimalPrice);
                        } else {
                            throw std::logic_error("Unknown Demand Type!");
                        }
                        assert(decision->prices[pricingEvent.relativeTime][iter->second].first
                                == itinerary.id);
                        if (ignoreSpot) {
                            demandValue = 0.;
                            if (demand.type == Demand::Type::linear) {
                                optimalPrice = -demand.additive / demand.multiplicative;
                            } else {
                                optimalPrice = 1e100;
                            }
                        } else {
                            Type localRevenue = (optimalPrice + yr) * demandValue;
                            functionValue += localRevenue;
                        }

                        decision->prices[pricingEvent.relativeTime][
                            iter->second].second = Value(optimalPrice);
                    }

                    functionValue += F_r * mu_r;

                    for (ui32 allotmentId : links.allotmentsWithItinerary[itinerary.id]) {
                        if (decision->allotmentAccepted[allotmentId]) {
                            const auto& allotment = input.allotments[allotmentId];
                            ui32 placeIndex = links.allotmentItineraryToPlace.at(
                                    allotment.id).at(itinerary.id);
                            assert(decision->allotmentContainersQ[
                                    allotmentId][placeIndex].first == itinerary.id);
                            // The following assert is OK only
                            // when we always change deterministic solution.
                            assert(decision->allotmentAccepted[allotmentId]
                                    || !decision->allotmentContainersQ[
                                    allotmentId][placeIndex].second);
                            ui32 F_r_i = decision->allotmentContainersQ[
                                allotmentId][placeIndex].second;
                            Type qri = qr;
                            const auto& entry = input.allotmentEntries[
                                links.allotmentItineraryToEntry.at(
                                        allotment.id).at(itinerary.id)];
                            qri += entry.price;
                            Type showMultiplier =
                                entry.showRate.estimatedProba * entry.productAmount;
                            Type objectiveAdd = std::max(Type(0.), Type(qri - mu_r));
                            Type addFunctionValue
                                = (showMultiplier *
                                        (-itinerary.declineCost - entry.cancellationPrice
                                         + objectiveAdd)
                                        + entry.productAmount * entry.cancellationPrice);
                            addFunctionValue += F_r_i * mu_r;
                            functionValue += addFunctionValue;

                        }
                    }
                }
            }
        }
    }

    vector<int> localContainersInPorts(input.ports.size(), 0);
    vector<int> backByArc(input.arcs.size(), 0);
    Type paidForContainers = 0.;
    for (const auto& event : input.events) {
        if (event.type == EventType::cutoff) {
            const auto& node = input.nodes[event.basedNode.value()];
            const auto& arc = input.arcs[event.basedArc.value()];
            const auto& port = input.ports[node.portId];
            localContainersInPorts[port.id] += decision->hiredY[arc.id];
            assert(localContainersInPorts[port.id] >= 0);
            if (event.relativeTime >= timeParameters.timeEvent) {
                paidForContainers += port.hiringCost * decision->hiredY[arc.id];
            }
            for (ui32 idItinerary : event.relatedItineraryIds) {
                const auto& itinerary = input.itineraries[idItinerary];
                const auto& lastArc = input.arcs[itinerary.orderedArcs.back()];
                ui32 takenOnItinerary = 0;
                takenOnItinerary += decision->nonEmptyContainersQ[itinerary.id];
                takenOnItinerary += decision->emptyContainersZ[itinerary.id];
                for (ui32 idAllotment : links.allotmentsWithItinerary[itinerary.id]) {
                    if (decision->allotmentAccepted[idAllotment]) {
                        ui32 place = links.allotmentItineraryToPlace.at(
                                idAllotment).at(itinerary.id);
                        takenOnItinerary += decision->allotmentContainersQ[
                            idAllotment][place].second;
                        assert(decision->allotmentContainersQ[idAllotment][place].first
                                == itinerary.id);
                    }
                }
                backByArc[lastArc.id] += takenOnItinerary;
                localContainersInPorts[port.id] -= takenOnItinerary;
                if (localContainersInPorts[port.id] < 0) {
                    decision->hiredY[arc.id] -= localContainersInPorts[port.id];
                    paidForContainers += port.hiringCost * (-localContainersInPorts[port.id]);
                    localContainersInPorts[port.id] = 0;
                }
            }

        } else if (event.type == EventType::arrival) {
            const auto& node = input.nodes[event.basedNode.value()];
            const auto& port = input.ports[node.portId];
            localContainersInPorts[node.portId] += backByArc[event.basedArc.value()];
            ui32 amount = decision->offHiredInPortS.at(event.relativeTime).at(port.id);
            if (localContainersInPorts[node.portId] < int(amount)) {
                amount = localContainersInPorts[node.portId];
            }
            localContainersInPorts[node.portId] -= amount;
            if (event.relativeTime >= timeParameters.timeEvent) {
                paidForContainers += port.offHiringCost * amount;
            }
        }
        if (event.relativeTime >= timeParameters.timeEvent) {
            for (const auto& port : input.ports) {
                paidForContainers +=
                    event.duration * port.storageCost * localContainersInPorts[port.id];
            }
        }
        for (const auto& port : input.ports) {
            assert(localContainersInPorts[port.id] >= 0);
        }
    }
    for (const auto& port : input.ports) {
        int containerDiff = port.finalContainerCount - localContainersInPorts[port.id];
        if (containerDiff >= 0) {
            paidForContainers += containerDiff * port.hiringCost;
        } else {
            paidForContainers -= containerDiff * port.offHiringCost;
        }
    }
    functionValue -= paidForContainers;
    return functionValue;
}

using NestedAD=CppAD::AD<CppAD::AD<double>>;

template<> inline NestedAD computeFunctionValue<NestedAD>(
        const InputData&, const InputLinks&, const State&,
        const LagrangianRelaxationIndex&, const TimeParameters&,
        Decision*, const DualTemplate<NestedAD>&, bool, double,
        DualTemplate<double>) {
    throw std::logic_error("Second derivative is not supported");
}

} // backend
} // sea
