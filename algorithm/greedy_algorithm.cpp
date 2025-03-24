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
#include "../logging/logging.h"
#include "../backend/baseline/plan.h"
#include "../backend/baseline/spot.h"
#include "../backend/baseline/long.h"
#include "../backend/baseline/api.h"


#include <cassert>
#include <memory>
#include <map>
#include <ranges>

namespace sea {
namespace algo {

using EventType=InputData::Event::Type;
using BaselineStats=backend::BaselineStats;
using ItineraryPlan=backend::spot::ItineraryPlan;

GreedyAlgorithm::GreedyAlgorithm(const GreedyConfig& aConfig)
        : allotmentsAsked(false)
        , trackStory(aConfig.trackStory)
        , input(aConfig.inputManager->getConstData())
        , links(aConfig.linksManager->getConstData())
        , ignoreSpotMarket(aConfig.ignoreSpotMarket)
        , ignoreLongMarket(aConfig.ignoreLongMarket)
        , memoryOptimization(aConfig.memoryOptimization) {
    backend::allotment::BaselineAllotmentConfig longSorterConfig(
            {aConfig.inputManager, aConfig.linksManager});
    allotmentSorter = std::make_unique<backend::allotment::LongCompositeSorter>(longSorterConfig);
    backend::spot::BaselineSpotConfig spotSorterConfig(
            {aConfig.inputManager, aConfig.linksManager});
    spotSorter = std::make_unique<backend::spot::CompositeSpotSorter>(spotSorterConfig);
    reset();
}

std::string GreedyAlgorithm::getName() const {
    return "Algorithm:<baseline_allotment,baseline_spot>";
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
    auto allotmentOrder = allotmentSorter->selectOrder();
    for (unsigned idxAllotment : allotmentOrder) {
        assert(idxAllotment < input.allotments.size());
        auto& allotment = input.allotments[idxAllotment];
        bool allotmentAvailable = checkIfAllotmentAvailable(allotment.id);
        double expectedProfit = backend::computeExpectedAllotmentProfit(
                input, links, idxAllotment);
        if (allotmentAvailable && expectedProfit > 0.) {
            decision.allotmentAccepted[allotment.id] = true;
            backend::updateStatsAtAllotmentSelection(&greedyStats, input, allotment.id);
            for (unsigned entryIdx : allotment.entries) {
                const auto& entry = input.allotmentEntries[entryIdx];
                unsigned routeIdx = entry.itinerary;
                state.acceptedAllotmentBookings[routeIdx] += entry.productAmount;
                state.expectedAllotmentCapacity[routeIdx]
                    += entry.productAmount * entry.showRate.estimatedProba;
            }
        } else {
            decision.allotmentAccepted[allotment.id] = false;
        }
    }
    logAllotmentSelection(decision.allotmentAccepted);
    return result;
}

void GreedyAlgorithm::logAllotmentSelection(const std::vector<bool>& acceptedAllotments) const {
    auto& logger = logging::getAlgorithmLogger();
    auto stream = logger.getStream(log4cpp::Priority::INFO);
    stream << "Greedy Algorithm. These are the accepted allotments: " << "\n";
    for (auto value : acceptedAllotments) {
        stream << value << " ";
    }
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
    std::map<unsigned, unsigned> itineraryIdToIndex;
    for (auto [index, idxRoute] : std::views::enumerate(event.relatedItineraryIds)) {
        itineraryIdToIndex[idxRoute] = index;
    }
    auto itineraryOrder = spotSorter->selectOrder(event, greedyStats);
    for (const auto& idxRoute: itineraryOrder) {
        unsigned idx = itineraryIdToIndex[idxRoute];
        assert(idxRoute == event.relatedItineraryIds[idx]);
        assert(prices[idx].first == idxRoute);
        const InputData::Itinerary& route = input.itineraries[idxRoute];
        auto demand = event.demands[idx];
        // Setting field value.
        ItineraryPlan optimalPlan = backend::spot::buildItineraryPlan(
                input, links, greedyStats, route, demand);
        prices[idx].second = optimalPlan.price;
        // Update greedy stats.
        double expectedAmount = optimalPlan.demand * route.showRate.estimatedProba;
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

