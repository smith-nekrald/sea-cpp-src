// Implements conceptual methods for Abstract SpotMarket Strategy.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
// (c) Smith School of Business, 2025

#include "../manager.h"
#include "../algorithm/state.h"
#include "abstract_spot_market_strategy.h"

#include <cassert>
#include <limits>
#include <stdexcept>
#include <map>
#include <cmath>


namespace sea {
namespace strategy {

using EventType=InputData::Event::Type;

AbstractSpotMarketStrategy::AbstractSpotMarketStrategy(
        const SpotMarketStrategyConfig& aConfig,
        const BackendConfigHolder& aBackendConfigs,
        const std::string aName)
            : backendConfigs(aBackendConfigs)
            , config(aConfig)
            , censor(aConfig.inputManager, aConfig.linksManager)
            , name(aName)
            , keepStory(false) {
    initialize();
    logCreated();
}

ConstDecisionManagerPtr AbstractSpotMarketStrategy::makeDecision() {
    logCalledMakeDecision();
    auto* decision = decisionManager->get();
    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();
    auto& timeParameters = state.timeParameters;
    decision->time = state.timeParameters.timeEvent;
    assert(timeParameters.allotmentsSupplied);
    assert(input.events.size() > timeParameters.timeEvent);
    const auto eventNow = input.events[timeParameters.timeEvent];
    if (eventNow.type == EventType::pricing) {
        if (!timeParameters.doneDecision) {
            assert(!timeParameters.doneAction);
            assert(!timeParameters.gotPortDecision);
            processPricing(eventNow);
            timeParameters.doneDecision = true;
        } else {
            assert(timeParameters.doneAction);
            assert(!timeParameters.gotPortDecision);
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
                state.carriedOnRoute[routeId] += decision->emptyContainersZ[routeId];
                for (unsigned contractId = 0; contractId < input.allotments.size(); ++contractId) {
                    if (links.allotmentItineraryToPlace[contractId].find(routeId) !=
                            links.allotmentItineraryToPlace[contractId].end()) {
                        unsigned placeIndex = links.allotmentItineraryToPlace[
                            contractId].at(routeId);
                        const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();
                        assert(placeIndex != MAX_INDEX);
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
        assert(!timeParameters.doneAction);
        assert(!timeParameters.doneDecision);
        assert(!timeParameters.gotPortDecision);
        processArrival(eventNow);
        processOffhiring(eventNow);
        timeParameters.gotPortDecision = true;
        toNextEvent(&timeParameters);
    } else {
        throw std::logic_error("This event type is not supported");
    }
    logFinishedMakeDecision();
    return decisionManager;
}

void AbstractSpotMarketStrategy::supplyAllotmentDecision(
        const DecisionManagerPtr& decisionManagerNew) {
    decisionManager = decisionManagerNew;
    initState(config.inputManager->getConstData(), &state);
    state.timeParameters.allotmentsSupplied = true;
}

void AbstractSpotMarketStrategy::submitAction(ConstActionManagerPtr newActionManager) {
    actionManager = newActionManager;
    auto* action = &newActionManager->getConstData();
    auto& timeParameters = state.timeParameters;
    const auto& input = config.inputManager->getConstData();
    assert(action->time >= timeParameters.timeEvent);
    assert(timeParameters.allotmentsSupplied);
    assert(input.events.size() > timeParameters.timeEvent);
    const auto& eventNow = input.events[timeParameters.timeEvent];
    assert(eventNow.type != EventType::arrival);
    assert(action->time == timeParameters.timeEvent);
    if (eventNow.type == EventType::cutoff) {
        assert(!timeParameters.doneDecision);
        assert(!timeParameters.doneAction);
        assert(!timeParameters.gotPortDecision);
        timeParameters.doneAction = true;
    } else if (eventNow.type == EventType::pricing) {
        assert(timeParameters.doneDecision);
        assert(!timeParameters.doneAction);
        assert(!timeParameters.gotPortDecision);
        timeParameters.doneAction = true;
        // update bookings in state
        for (unsigned idx = 0; idx < action->bookingsB[eventNow.relativeTime].size(); ++idx) {
            state.accumulatedBookings[idx] += action->bookingsB[eventNow.relativeTime][idx];
        }
    } else {
        throw std::logic_error("This event type is not supported");
    }
}

void AbstractSpotMarketStrategy::processPricing(const InputData::Event& event) {
    updateParams(event);
    censor.correctPricing(event, state, decisionManager);
}

void AbstractSpotMarketStrategy::processArrival(const InputData::Event& event) {
    updateParams(event);
    const auto& input = config.inputManager->getConstData();
    assert(event.type == EventType::arrival);
    const auto& arc = input.arcs[event.basedArc.value()];
    const auto& node = input.nodes[arc.toNode];
    const auto& port = input.ports[node.portId];
    auto* decision = &decisionManager->getConstData();
    auto& containersInPorts = state.containersInPorts;
    const auto& links = config.linksManager->getConstData();
    for (unsigned itineraryId : event.relatedItineraryIds) {
        containersInPorts[port.id] += decision->emptyContainersZ[itineraryId];
        containersInPorts[port.id] += decision->nonEmptyContainersQ[itineraryId];
        for (unsigned allotmentId : links.allotmentsWithItinerary[itineraryId]) {
            if (decision->allotmentAccepted[allotmentId]) {
                unsigned place = links.allotmentItineraryToPlace[allotmentId].at(itineraryId);
                containersInPorts[port.id] += decision->allotmentContainersQ[
                    allotmentId][place].second;
            }
        }
    }
}

void AbstractSpotMarketStrategy::processOffhiring(const InputData::Event& event) {
    updateParams(event);
    unsigned eventTime = event.relativeTime;
    auto* decision = decisionManager->get();
    auto& containersInPorts = state.containersInPorts;
    const auto& input = config.inputManager->getConstData();
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort)  {
        int offHired = static_cast<int>(decision->offHiredInPortS[eventTime][idPort]);
        if (offHired > containersInPorts[idPort]) {
            assert(containersInPorts[idPort] >= 0);
            decision->offHiredInPortS[eventTime][idPort] = containersInPorts[idPort];
        }
        int finalCount = static_cast<int>(input.ports[idPort].finalContainerCount);
        if (event.relativeTime + 1 == input.events.size()) {
            if (containersInPorts[idPort] >= finalCount) {
                decision->offHiredInPortS[eventTime][idPort] =
                    containersInPorts[idPort] - input.ports[idPort].finalContainerCount;
            }
        }
        containersInPorts[idPort] -= decision->offHiredInPortS[eventTime][idPort];
        assert(containersInPorts[idPort] >= 0);
    }
}

void AbstractSpotMarketStrategy::initialize() {
    const double INF = std::numeric_limits<double>::max();
    lastUpdate = -INF;
    story.clear();
    state = State();
    initState(config.inputManager->getConstData(), &state);
    const auto& events = config.inputManager->getConstData().events;
    if (events.size() && config.moveUpdateInterval) {
        lastUpdate = events[0].realTime;
    }
    decisionManager = nullptr;
    actionManager = nullptr;
}

void AbstractSpotMarketStrategy::hardReset() {
    backends.lrBackend = nullptr;
    backends.ipoptBackend = nullptr;
    backends.bendersBackend = nullptr;
    reset();
}

} // namespace strategy
} // namespace sea
