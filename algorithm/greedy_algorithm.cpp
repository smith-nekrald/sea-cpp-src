// Implements Greedy Algorithm logic.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
// (c) Smith School of Business, 2025
//

#include "greedy_algorithm.h"
#include "../interaction/protocol.h"
#include "../manager.h"

#include <cmath>
#include <cassert>
#include <memory>

namespace sea {
namespace algo {

using EventType=InputData::Event::Type;
using BaselineStats=backend::BaselineStats;

GreedyAlgorithm::GreedyAlgorithm(const GreedyConfig& aConfig)
        : allotmentsAsked(false)
        , trackStory(aConfig.trackStory)
        , input(aConfig.inputManager->getConstData())
        , links(aConfig.linksManager->getConstData())
        , ignoreSpotMarket(aConfig.ignoreSpotMarket)
        , ignoreLongMarket(aConfig.ignoreLongMarket)
        , memoryOptimization(aConfig.memoryOptimization) {
    backend::allotment::AllotmentSorterConfig sorterConfig(
            {aConfig.inputManager, aConfig.linksManager});
    sorter = std::make_unique<backend::allotment::LongCompositeSorter>(sorterConfig);
    reset();
}

std::string GreedyAlgorithm::getName() const {
    return "Algorithm:<fcfs,greedy>";
}

void GreedyAlgorithm::submitAction(ConstActionManagerPtr newManager) {
    assert(allotmentsAsked);
    actionManager = newManager;
    auto* action = &actionManager->getConstData();
    auto& timeParameters = state.timeParameters;
    assert(action->time >= timeParameters.timeEvent && timeParameters.allotmentsSupplied
            && input.events.size() > timeParameters.timeEvent);
    const auto& eventNow = input.events[timeParameters.timeEvent];
    assert(eventNow.type != EventType::arrival && action->time == timeParameters.timeEvent);
    if (eventNow.type == EventType::cutoff) {
        assert(!timeParameters.doneDecision && !timeParameters.doneAction
                && !timeParameters.gotPortDecision);
        timeParameters.doneAction = true;
    } else if (eventNow.type == EventType::pricing) {
        assert(timeParameters.doneDecision && !timeParameters.doneAction
                && !timeParameters.gotPortDecision);
        timeParameters.doneAction = true;
        // update bookings in state
        for (unsigned idx = 0; idx < action->bookingsB[eventNow.relativeTime].size(); ++idx) {
            state.accumulatedBookings[idx] += action->bookingsB[eventNow.relativeTime][idx];
        }
    } else {
        throw std::logic_error("This event type is not supported!");
    }
}

ConstDecisionManagerPtr GreedyAlgorithm::makeDecision() {
    if (!allotmentsAsked) {
        decisionManager = provideAllotments();
        allotmentsAsked = true;
        state.timeParameters.allotmentsSupplied = true;
    } else{
        decisionManager = makeSpotDecision();
    }
    return decisionManager;
}

void GreedyAlgorithm::reset() {
    allotmentsAsked = false;
    backend::initBaselineStats(&greedyStats, input);
    state = State();
    initState(input, &state);
    story.clear();
    decisionManager = nullptr;
    actionManager = nullptr;
}

void GreedyAlgorithm::synchronizeStrategies() {}

std::map<std::string, std::vector<double>> GreedyAlgorithm::getStory() const {
    return story;
}

double GreedyAlgorithm::computeGreedyCapacityForItinerary(size_t idxItinerary) const {
    return backend::computeExpectedAvailableCapacityForItinerary(input, greedyStats, idxItinerary);
}

size_t GreedyAlgorithm::computeAllocationCapacityForItinerary(size_t idxRoute) const {
    return backend::computeExactAllocationCapacityForItinerary(input, greedyStats, idxRoute);
}

void GreedyAlgorithm::allocateCapacityForItinerary(size_t idxRoute, size_t amount) {
    backend::allocatePreciseCapacityForItinerary(&greedyStats, input, idxRoute, amount);
}

bool GreedyAlgorithm::checkIfAllotmentAvailable(size_t idxAllotment) const {
    return backend::checkIfAllotmentAvailable(input, greedyStats, idxAllotment);
}

DecisionManagerPtr GreedyAlgorithm::provideAllotments() {
    std::string filePath = "decision_" + makeUniqueFileName();
    ManagerConfig decisionConfig = {memoryOptimization, filePath, true};
    DecisionManagerPtr result = std::make_shared<DataManager<Decision>>(decisionConfig);
    auto& decision = result->getData();
    createDecision(input, &decision);
    if (ignoreLongMarket) {
        return result;
    }
    auto allotmentOrder = sorter->selectOrder();
    for (unsigned idxAllotment : allotmentOrder) {
        auto& allotment = input.allotments[idxAllotment];
        bool allotmentAvailable = checkIfAllotmentAvailable(allotment.id);
        decision.allotmentAccepted[allotment.id] = allotmentAvailable;
        if (allotmentAvailable) {
            backend::updateStatsAtAllotmentSelection(&greedyStats, input, allotment.id);
        }
    }
    return result;
}

DecisionManagerPtr GreedyAlgorithm::makeSpotDecision() {
    auto* decision = decisionManager->get();
    auto& timeParameters = state.timeParameters;
    decision->time = state.timeParameters.timeEvent;
    assert(timeParameters.allotmentsSupplied && input.events.size() > timeParameters.timeEvent);

    const auto eventNow = input.events[timeParameters.timeEvent];
    if (eventNow.type == EventType::pricing) {
        if (!timeParameters.doneDecision) {
            assert(!timeParameters.doneAction && !timeParameters.gotPortDecision);
            processPricing(eventNow);
            timeParameters.doneDecision = true;
        } else {
            assert(timeParameters.doneAction && !timeParameters.gotPortDecision);
            processOffhiring(eventNow);
            timeParameters.gotPortDecision = true;
            toNextEvent(&timeParameters);
        }
    } else if (eventNow.type == EventType::cutoff) {
        if (!timeParameters.doneDecision) {
            assert(timeParameters.doneAction);
            assert(!timeParameters.gotPortDecision);
            processCutoff(eventNow);
            timeParameters.doneDecision = true;
            // update state
            for (unsigned routeId : eventNow.relatedItineraryIds) {
                assert(state.carriedOnRoute[routeId] == 0);
                state.carriedOnRoute[routeId] += decision->nonEmptyContainersQ[routeId];
                assert(decision->emptyContainersZ[routeId] == 0);
                for (unsigned contractId = 0; contractId < input.allotments.size(); ++contractId) {
                    if (links.allotmentItineraryToPlace[contractId].find(routeId) !=
                            links.allotmentItineraryToPlace[contractId].end()) {
                        unsigned placeIndex = links.allotmentItineraryToPlace[
                            contractId].at(routeId);
                        assert(placeIndex != std::numeric_limits<unsigned>::max());
                        assert(routeId == decision->allotmentContainersQ[
                                contractId][placeIndex].first);
                        auto amountQ = decision->allotmentContainersQ[
                            contractId][placeIndex].second;
                        state.carriedOnRoute[routeId] += amountQ;
                    }
                }
            }
        } else {
            assert(timeParameters.doneAction);
            assert(!timeParameters.gotPortDecision);
            processOffhiring(eventNow);
            timeParameters.gotPortDecision = true;
            toNextEvent(&timeParameters);
        }
    } else if (eventNow.type == EventType::arrival) {
        assert(!timeParameters.doneAction && !timeParameters.doneDecision
               && !timeParameters.gotPortDecision);
        processArrival(eventNow);
        processOffhiring(eventNow);
        timeParameters.gotPortDecision = true;
        toNextEvent(&timeParameters);
    } else {
        throw std::logic_error("This event type is not supported.");
    }
    return decisionManager;
}

void GreedyAlgorithm::processCutoff(const InputData::Event& event) {
    auto* decision = decisionManager->get();
    auto* action = &actionManager->getConstData();
    const auto& arc = input.arcs[event.basedArc.value()];
    decision->hiredY[arc.id] = 0;
    for (unsigned idItinerary : event.relatedItineraryIds) {
        // Process Spot Market.
        unsigned shownCountSpot = action->spotMarketDemandN[idItinerary];
        decision->emptyContainersZ[idItinerary] = 0;
        size_t maxAvailableSpot = computeAllocationCapacityForItinerary(idItinerary);
        size_t allocateSpot = std::min<size_t>(shownCountSpot, maxAvailableSpot);
        decision->nonEmptyContainersQ[idItinerary] = allocateSpot;
        allocateCapacityForItinerary(idItinerary, allocateSpot);
        decision->hiredY[arc.id] += allocateSpot;
        auto& route = input.itineraries[idItinerary];
        // Update Greedy Stats with new spot-related information.
        double allocatedGreedy = greedyStats.allocatedSpotRouteCapacity[route.id];
        for (unsigned idxArc: route.orderedArcs) {
            greedyStats.availableArcCapacity[idxArc] += allocatedGreedy;
            greedyStats.availableArcCapacity[idxArc] -= allocateSpot;
        }
        greedyStats.allocatedSpotRouteCapacity[route.id] = allocateSpot;
        // Process Allotments.
        for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
            if (decision->allotmentAccepted[idAllotment]) {
                unsigned placeIndex = links.allotmentItineraryToPlace.at(
                        idAllotment).at(idItinerary);
                unsigned shownAllotment = action->allotmentDemandN[idAllotment][placeIndex].second;
                size_t maxAvailableLong = computeAllocationCapacityForItinerary(idItinerary);
                size_t allocateLong = std::min<size_t>(shownAllotment, maxAvailableLong);
                decision->allotmentContainersQ[idAllotment][placeIndex].second = allocateLong;
                allocateCapacityForItinerary(idItinerary, allocateLong);
                assert(action->allotmentDemandN[idAllotment][placeIndex].first == idItinerary);
                assert(decision->allotmentContainersQ[idAllotment][placeIndex].first
                        == idItinerary);
                decision->hiredY[arc.id] += allocateLong;
                // Update Greedy Stats with new allotment-related information.
                unsigned entryId = links.allotmentItineraryToEntry[idAllotment].at(route.id);
                double allocatedFCFS = greedyStats.allocatedLongEntryCapacity[entryId];
                for (unsigned idxArc: route.orderedArcs) {
                    greedyStats.availableArcCapacity[idxArc] += allocatedFCFS;
                    greedyStats.availableArcCapacity[idxArc] -= allocateLong;
                }
                greedyStats.allocatedLongEntryCapacity[entryId] = allocateLong;
            }
        }
    }
}

void GreedyAlgorithm::processPricing(const InputData::Event& event) {
    assert(event.type == InputData::Event::Type::pricing);
    assert(event.demands.size() == event.relatedItineraryIds.size());
    auto& decision = decisionManager->getData();
    auto& prices = decision.prices[event.relativeTime];
    size_t nPricesToSet = event.demands.size();
    const double INF = std::numeric_limits<double>::max();
    if (ignoreSpotMarket) {
        for (size_t idx = 0; idx < nPricesToSet; ++idx) {
            prices[idx].second = INF;
        }
        return;
    }
    for (size_t idx = 0; idx < nPricesToSet; ++idx) {
        // Find maximal available capacity.
        size_t idxRoute = event.relatedItineraryIds[idx];
        const InputData::Itinerary& route = input.itineraries[idxRoute];
        assert(prices[idx].first == idxRoute);
        auto demand = event.demands[idx];
        double freeCapacity = computeGreedyCapacityForItinerary(idxRoute);
        double maxCapacity = freeCapacity / route.showRate.estimatedProba;
        const double ONE_TEU = 1.;
        if (maxCapacity < ONE_TEU) {
            prices[idx].second = INF;
            continue;
        }
        // Compute complete shipping cost.
        double shippingCost = route.cost;

        size_t idxStartArc = route.orderedArcs.front();
        const InputData::Arc& startArc = input.arcs[idxStartArc];
        const InputData::Node& startNode = input.nodes[startArc.fromNode];
        const InputData::Port& startPort = input.ports[startNode.portId];
        shippingCost += startPort.hiringCost;
        shippingCost += links.itineraryIdToCutoffDuration[idxRoute] * startPort.storageCost;

        size_t idxFinalArc = route.orderedArcs.back();
        const InputData::Arc& finalArc = input.arcs[idxFinalArc];
        const InputData::Node& finalNode = input.nodes[finalArc.toNode];
        const InputData::Port& finalPort = input.ports[finalNode.portId];
        shippingCost += finalPort.offHiringCost;


        // Find optimal price and demand.
        double optimalDemand = 0.;
        double optimalPrice = INF;
        const double EPS = 1e-3;
        if (demand.type == Demand::Type::exponential) {
            optimalDemand = demand.scale / std::exp(
                    route.showRate.estimatedProba
                        * (1 + shippingCost * demand.sensitivity)
                    + (1 - route.showRate.estimatedProba)
                        * (1 + route.returnPrice * demand.sensitivity));
            optimalDemand = std::min(optimalDemand, maxCapacity);
            const double LOG_EPS = 1e-50;
            optimalPrice = 1. / demand.sensitivity * std::log(
                    LOG_EPS + demand.scale / optimalDemand);
            assert(optimalDemand <= demand.scale + EPS);
        } else if (demand.type == Demand::Type::linear) {
            assert(demand.multiplicative < 0.);
            optimalPrice = 0.5 * (
                    shippingCost * route.showRate.estimatedProba
                    + route.returnPrice * (1 - route.showRate.estimatedProba)
                    - demand.additive / demand.multiplicative);
            optimalPrice = std::max(optimalPrice,
                    (demand.additive - maxCapacity) / (-demand.multiplicative));
            optimalPrice = std::min(optimalPrice, demand.additive / (-demand.multiplicative));
            optimalDemand = demand.additive + optimalPrice * demand.multiplicative;
            assert(optimalDemand + EPS >= 0. && optimalDemand <= demand.additive + EPS);
        } else {
            throw std::logic_error("Unexpected demand type!");

        }
        // Robustness in prices and demand.
        optimalPrice += EPS;
        optimalDemand += EPS;
        assert(optimalPrice >= 0);

        // Revenue-based verification.
        double expectedRevenue
            = route.showRate.estimatedProba * optimalDemand
                * (optimalPrice - shippingCost)
            + (1 - route.showRate.estimatedProba) * optimalDemand
                * (optimalPrice - route.returnPrice);
        if (expectedRevenue + EPS < 0.) {
            optimalPrice = INF;
            optimalDemand = EPS;
        }

        // Setting field values.
        prices[idx].second = optimalPrice;

        // Update greedy stats.
        double expectedAmount = optimalDemand * route.showRate.estimatedProba;
        for (unsigned idxArc : route.orderedArcs) {
            greedyStats.availableArcCapacity[idxArc] -= expectedAmount;
        }
        greedyStats.allocatedSpotRouteCapacity[route.id] += expectedAmount;
    }
}

void GreedyAlgorithm::processArrival(const InputData::Event& event) {
    assert(event.type == EventType::arrival);
    const auto& arc = input.arcs[event.basedArc.value()];
    const auto& node = input.nodes[arc.toNode];
    const auto& port = input.ports[node.portId];
    const auto& decision = decisionManager->getConstData();
    auto& containersInPorts = state.containersInPorts;
    for (unsigned itineraryId : event.relatedItineraryIds) {
        assert(decision.emptyContainersZ[itineraryId] == 0);
        containersInPorts[port.id] += decision.nonEmptyContainersQ[itineraryId];
        for (unsigned allotmentId : links.allotmentsWithItinerary[itineraryId]) {
            if (decision.allotmentAccepted[allotmentId]) {
                unsigned place = links.allotmentItineraryToPlace[allotmentId].at(itineraryId);
                containersInPorts[port.id] += decision.allotmentContainersQ[
                    allotmentId][place].second;
            }
        }
    }
}

void GreedyAlgorithm::processOffhiring(const InputData::Event& event) {
    unsigned eventTime = event.relativeTime;
    auto* decision = decisionManager->get();
    auto& containersInPorts = state.containersInPorts;
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        // Since the policy is greedy, offhire everything.
        decision->offHiredInPortS[eventTime][idPort] = containersInPorts[idPort];
        assert(input.ports[idPort].finalContainerCount == 0);
        containersInPorts[idPort] = 0;
        assert(containersInPorts[idPort] >= 0);
    }
}

} // namespace algo
} // namespace sea

