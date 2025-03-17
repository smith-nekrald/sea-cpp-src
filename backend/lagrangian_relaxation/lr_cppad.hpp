/**
 * @file lr_cppad.hpp
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Template implementation for computing LR objective in point. Supported types:
 * double and CppAD::AD<double>.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "lagrangian_relaxation_backend.h"
#include "index.h"
#include "lr_cppad.h"
#include "functions.h"

#include <stdexcept>
#include <cppad/cppad.hpp>
#include <cppad/core/value.hpp>
#include <cppad/core/var2par.hpp>

namespace sea {
namespace backend {

using ArcType=InputData::Arc::Type;
using EventType=InputData::Event::Type;
using CppAD::AD;


/**
 * @brief Computes function value (or expression) in point. Two types are supported: CppAD<double>
 * and double. Uniform way to compute function value, function expression (and therefore
 * subgradient).
 *
 * @tparam Type The expression type. Expected options: double and CppAD<double>.
 * @param[in] input InputData with environmental statistics.
 * @param[in] links InputLinks, a data structure for effective work with InputData.
 * @param[in] state The state of trajectory evaluation.
 * @param[in] lrIndex Lagrangian Relaxation index.
 * @param[in] timeParameters Time parameters, allow to track current pivot event and stage.
 * @param[out] decision The Decision to form.
 * @param[in] point Point with dual variables.
 * @param[in] ignoreSpot If true, spot market is ignored.
 * @param[in] l2RegConstant Coefficient for l2 regularization. Only relevant for cutting plane,
 * gradient methods UPGM and UFGM have their own regularizations.
 * @param[in] mean Mean value for centering. Note that UFGM and UPGM have their own centering.
 * @return Function in point, expression or value.
 */
template <typename Type>
inline Type computeFunctionValue(const InputData& input,
                   const InputLinks& links,
                   const State& state,
                   const LagrangianRelaxationIndex& lrIndex,
                   const TimeParameters& timeParameters,
                   Decision* decision,
                   const DualTemplate<Type>& point,
                   bool ignoreSpot = false,
                   double l2RegConstant = 0,
                   DualVariables mean = DualVariables()){
    Type functionValue = static_cast<Type>(0);
    const double EPS = 1e-7;
    if (l2RegConstant > EPS) {
        Type regularizer = static_cast<Type>(0);
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
    for (unsigned idArc = 0; idArc < input.arcs.size(); ++idArc) {
        const auto& arc = input.arcs[idArc];
        Type arcFromStartTime = input.nodes[arc.fromNode].realTime;
        if (arc.type == ArcType::travel && arcFromStartTime >= theNextEvent.realTime) {
            const auto& vessel = input.vessels[arc.vesselId.value()];
            unsigned place = lrIndex.arcToLambdaIndex.at(idArc);
            Type scaledCapacity = vessel.capacity * point.lambdaVariables[place];
            scaledCapacitySum += scaledCapacity;
        }
    }
    functionValue += scaledCapacitySum;
    for (const auto& event: input.events) {
        if (event.type == EventType::cutoff) {
            // Scenario on events that have already passed.
            if (event.relativeTime < timeParameters.timeEvent) {
                for (unsigned itineraryId : event.relatedItineraryIds) {
                    const auto& itinerary = input.itineraries[itineraryId];
                    for (unsigned idArc : itinerary.orderedArcs) {
                        const auto& arc = input.arcs[idArc];
                        if (arc.type == ArcType::travel) {
                            const auto& arcNode = input.nodes[arc.fromNode];
                            double timeArc = arcNode.realTime;
                            if (timeArc >= theNextEvent.realTime) {
                                unsigned place = lrIndex.arcToLambdaIndex.at(idArc);
                                Type shipped = state.carriedOnRoute[itinerary.id];
                                functionValue -= shipped * point.lambdaVariables[place];
                            }
                        }
                    }
                }
            // Scenario for cutoffs that have not happened yet.
            } else {
                const auto& basedArc = input.arcs[event.basedArc.value()];
                for (unsigned itineraryId : event.relatedItineraryIds) {
                    const auto& itinerary = input.itineraries[itineraryId];
                    unsigned F_r = decision->nonEmptyContainersQ[itinerary.id]
                        + decision->emptyContainersZ[itinerary.id];
                    Type lambdaSum = 0.;
                    for (unsigned idArc : itinerary.orderedArcs) {
                        const auto& arc = input.arcs[idArc];
                        if (arc.type == ArcType::travel) {
                            lambdaSum += point.lambdaVariables[
                                lrIndex.arcToLambdaIndex.at(idArc)];
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

                    Type plusDiff = std::max<Type>(0., qr - mu_r);
                    Type yr = -itinerary.returnPrice + itinerary.showRate.estimatedProba * (
                                -itinerary.declineCost + itinerary.returnPrice + plusDiff);

                    functionValue += yr * state.accumulatedBookings[itinerary.id];

                    const auto& demandPlaces = links.demandTimesForItinerary[itinerary.id];
                    pair<unsigned, unsigned> targetPair = std::make_pair(
                            timeParameters.timeEvent, unsigned(0));
                    const auto& placeBegin = std::lower_bound(
                            demandPlaces.begin(), demandPlaces.end(), targetPair);

                    for (auto iter = placeBegin; iter != demandPlaces.end(); ++iter) {
                        const auto& pricingEvent = input.events[iter->first];
                        assert(pricingEvent.type == EventType::pricing);
                        assert(pricingEvent.relatedItineraryIds[iter->second] == itinerary.id);
                        const auto& demand = input.events[iter->first].demands[iter->second];
                        unsigned idxRoute = pricingEvent.relatedItineraryIds[iter->second];
                        const auto& route = input.itineraries[idxRoute];
                        double bottleneckDemand = computeItineraryBottleneck(
                                input, links, state, timeParameters.timeEvent, idxRoute)
                                /  route.showRate.estimatedProba;
                        Type optimalPrice = 0., demandValue = 0.;
                        if (demand.type == Demand::Type::linear) {
                            assert(demand.additive > 0. && demand.multiplicative < 0.);
                            Type leftPrice = 0.,
                                 rightPrice = -demand.additive / demand.multiplicative;
                            leftPrice = std::max<Type>(leftPrice, Type(itinerary.returnPrice));
                            rightPrice = std::min<Type>(
                                    rightPrice, -bottleneckDemand / demand.multiplicative);
                            rightPrice = std::max<Type>(leftPrice, rightPrice);

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
                            demandValue = std::max<Type>(Type(0.), demand.additive
                                + optimalPrice * demand.multiplicative);
                        } else if (demand.type == Demand::Type::exponential) {
                            assert(demand.scale > 0. && demand.sensitivity > 0.);
                            Type LOG_EPS = 1e-100;
                            Type minPrice = std::max<Type>({
                                    Type(1. / demand.sensitivity * log(
                                            (demand.scale + LOG_EPS)
                                            / (bottleneckDemand + LOG_EPS))),
                                    Type(0.),
                                    Type(itinerary.returnPrice)
                            });
                            optimalPrice = std::max<Type>(
                                    minPrice, 1. / demand.sensitivity - yr);
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
                                optimalPrice = std::max<Type>(
                                        optimalPrice, Type(itinerary.returnPrice));
                            } else {
                                const double INF = std::numeric_limits<double>::max();
                                optimalPrice = INF;
                            }
                        } else {
                            Type localRevenue = (optimalPrice + yr) * demandValue;
                            functionValue += localRevenue;
                        }

                        decision->prices[pricingEvent.relativeTime][
                            iter->second].second = Value(optimalPrice);
                    }

                    functionValue += F_r * mu_r;

                    for (unsigned allotmentId : links.allotmentsWithItinerary[itinerary.id]) {
                        if (decision->allotmentAccepted[allotmentId]) {
                            const auto& allotment = input.allotments[allotmentId];
                            unsigned placeIndex = links.allotmentItineraryToPlace.at(
                                    allotment.id).at(itinerary.id);
                            assert(decision->allotmentContainersQ[
                                    allotmentId][placeIndex].first == itinerary.id);
                            // The following assert is OK only
                            // when we always change deterministic solution.
                            assert(decision->allotmentAccepted[allotmentId]
                                    || !decision->allotmentContainersQ[
                                    allotmentId][placeIndex].second);
                            unsigned F_r_i = decision->allotmentContainersQ[
                                allotmentId][placeIndex].second;
                            Type qri = qr;
                            const auto& entry = input.allotmentEntries[
                                links.allotmentItineraryToEntry.at(
                                        allotment.id).at(itinerary.id)];
                            qri += entry.price;
                            Type showMultiplier =
                                entry.showRate.estimatedProba * entry.productAmount;
                            Type objectiveAdd = std::max<Type>(Type(0.), qri - mu_r);
                            Type addFunctionValue = (showMultiplier *
                                (-itinerary.declineCost - entry.cancellationPrice
                                + objectiveAdd) + entry.productAmount * entry.cancellationPrice);
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
            for (unsigned idItinerary : event.relatedItineraryIds) {
                const auto& itinerary = input.itineraries[idItinerary];
                const auto& lastArc = input.arcs[itinerary.orderedArcs.back()];
                unsigned carriedOnItinerary = 0;
                carriedOnItinerary += decision->nonEmptyContainersQ[itinerary.id];
                carriedOnItinerary += decision->emptyContainersZ[itinerary.id];
                for (unsigned idAllotment : links.allotmentsWithItinerary[itinerary.id]) {
                    if (decision->allotmentAccepted[idAllotment]) {
                        unsigned place = links.allotmentItineraryToPlace.at(
                                idAllotment).at(itinerary.id);
                        carriedOnItinerary += decision->allotmentContainersQ[
                            idAllotment][place].second;
                        assert(decision->allotmentContainersQ[idAllotment][place].first
                                == itinerary.id);
                    }
                }
                backByArc[lastArc.id] += carriedOnItinerary;
                localContainersInPorts[port.id] -= carriedOnItinerary;
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
            unsigned amount = decision->offHiredInPortS.at(event.relativeTime).at(port.id);
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

/// @brief Simplified notation for CppAD::AD<CppAD::AD<double>>.
using NestedAD=CppAD::AD<CppAD::AD<double>>;

/**
 * @brief Implementation specification for NestedAD. This method is never called, but
 * needs specification for compiling the template-based functions.
 *
 * @tparam NestedAD The template parameter specification.
 * @param input InputData with environmental statistics.
 * @param links InputLinks, a data structure for effective work with InputData.
 * @param state The state of trajectory evaluation.
 * @param lrIndex Lagrangian Relaxation index.
 * @param timeParameters Time parameters, allow to track current pivot event and stage.
 * @param decision The Decision to form.
 * @param point Point with dual variables.
 * @param ignoreSpot If true, spot market is ignored.
 * @param l2RegConstant Coefficient for l2 regularization. Only relevant for cutting plane,
 * gradient methods UPGM and UFGM have their own regularizations.
 * @param mean Mean value for centering. Note that UFGM and UPGM have their own centering.

 * @return Originally, expected to provide function value in point. Really, this specification
 * throws an exception, since second derivative is not needed and is not supported.
 */
template<> inline NestedAD computeFunctionValue<NestedAD>(
        [[maybe_unused]] const InputData& input,
        [[maybe_unused]] const InputLinks& links,
        [[maybe_unused]] const State& state,
        [[maybe_unused]] const LagrangianRelaxationIndex& lrIndex,
        [[maybe_unused]] const TimeParameters& timeParameters,
        [[maybe_unused]] Decision* decision,
        [[maybe_unused]] const DualTemplate<NestedAD>& point,
        [[maybe_unused]] bool ignoreSpot,
        [[maybe_unused]] double l2RegConstant,
        [[maybe_unused]] DualTemplate<double> mean) {
    throw std::logic_error("Second derivative is not supported");
}

} // backend
} // sea
