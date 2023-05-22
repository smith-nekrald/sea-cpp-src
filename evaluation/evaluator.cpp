#include "evaluator.h"
#include "../logging/logging.h"

#include <cassert>
#include <algorithm>
#include <sstream>
#include <iostream>

namespace sea {

using std::cout;
using std::endl;

using Event = InputData::Event;
using EventType = Event::Type;

Evaluator::Evaluator(const EvaluatorConfig& aConfig) : config(aConfig) {
    const auto& input = config.inputManager->getConstData();

    for (const auto & itinerary : input.itineraries) {
        const auto & arc = input.arcs[itinerary.orderedArcs.back()];
        arcEndsInPort.insert({input.nodes[arc.toNode].portId, arc.id});
        itinerariesEndInArc[arc.id].push_back(itinerary.id);
    }

    containersInPorts.resize(input.ports.size());
    containersAssignedOnItineraries.resize(input.itineraries.size());
    totalBookings.resize(input.itineraries.size());
    usedCapacity.resize(input.arcs.size(), 0);
}

void Evaluator::initStartState() {
    const auto& input = config.inputManager->getConstData();
    statistics = Statistics();
    selectedAllotments.clear();

    std::string filePath = "action_" + makeUniqueFileName();
    ManagerConfig actionConfig = {config.needMemory.value(), filePath, true};
    actionManager = std::make_shared<DataManager<Action>>(actionConfig);
    createAction(config.inputManager->getConstData(), actionManager->get());

    containersInPorts.assign(input.ports.size(), 0);
    std::transform(std::begin(input.ports), std::end(input.ports), std::begin(containersInPorts),
            [](const InputData::Port& port) { return port.initialContainerCount; });
    containersAssignedOnItineraries.assign(input.itineraries.size(), 0);
    totalBookings.assign(input.itineraries.size(), 0);
    usedCapacity.assign(input.arcs.size(), 0);

    evalStory.clear();
    algoStory.clear();
}

Statistics Evaluator::calc(algo::IAlgorithmPtr algo, ConstMarketManagerPtr market) {
    marketManager = market;
    initStartState();

    decisionManager = algo->makeDecision();   // time = -1, make decision for allotments
    logAllotmentDecision();

    processAllotmentDecision();
    const sea::InputData* inputPtr = config.inputManager->get();

    for (unsigned idEvent = 0;
            idEvent < config.inputManager->getConstData().events.size(); idEvent++) {

        inputPtr = config.inputManager->get();
        auto event = inputPtr->events[idEvent];
        logEvaluatorLaunched(event);

        // Stage 1. Pricing, cut-off or arrival.
        if (event.type == Event::Type::pricing) {
            logAboutPricing();
            decisionManager = algo->makeDecision();
            assert(decisionManager->getConstData().allotmentAccepted == selectedAllotments);
            makePricingAction(event);
            algo->submitAction(actionManager);
        } else if (event.type == Event::Type::cutoff) {
            logAboutCutoff();
            makeCutoffAction(event);
            algo->submitAction(actionManager);
            decisionManager = algo->makeDecision();
            assert(decisionManager->getConstData().allotmentAccepted == selectedAllotments);
            processCutoffDecision(event);
        } else {
            logAboutArrival();
            processArrival(event);
        }

        // Stage 2, get containers off-hired in ports
        logAboutOffhiring();
        decisionManager = algo->makeDecision();
        assert(decisionManager->getConstData().allotmentAccepted == selectedAllotments);
        inputPtr = config.inputManager->get();
        if (idEvent + 1 != inputPtr->events.size()) {
            auto dur = inputPtr->events[idEvent].realTime - event.realTime;
            assert(dur >= 0);
            updateContainersInPorts(dur);
        } else {
            updateContainersInPorts(0);
            checkFinalContainersInPorts();
        }
        logSpotProfitAfterLastEvent(event);
        statistics.fullProfit = statistics.spotProfit + statistics.allotmentProfit
            + statistics.containerProfit + statistics.emptyContainerProfit;

        if (config.keepStory) {
            evalStory["event_relative_time"].push_back(event.relativeTime);
            evalStory["event_real_time"].push_back(event.realTime);
            evalStory["profit_now"].push_back(statistics.fullProfit);
            evalStory["spot_profit_now"].push_back(statistics.spotProfit);
            evalStory["allotment_profit_now"].push_back(statistics.allotmentProfit);
            evalStory["container_profit_now"].push_back(statistics.containerProfit);
            evalStory["empty_container_profit_now"].push_back(statistics.emptyContainerProfit);
            evalStory["empty_containers_count_now"].push_back(statistics.emptyContainerCount);
            evalStory["spot_bookings_count_now"].push_back(statistics.sumSpotBookingAmount);
            evalStory["allotment_booking_count_now"].push_back(
                    statistics.sumAllotmentBookingAmount);
            evalStory["spot_container_count_now"].push_back(statistics.spotContainerCount);
            evalStory["long_container_count_now"].push_back(statistics.allotmentContainerCount);
        }
    }

    logProfits();
    logUsedCapacity();

    if (config.keepStory) {
        algoStory = algo->getStory();
        auto event = config.inputManager->get()->events[0];
        algoStory["start_real_time"].push_back(event.realTime);
    }

    return statistics;
}

void Evaluator::processArrival(const InputData::Event& event) {
    assert(event.type == EventType::arrival);
    const auto& input = config.inputManager->getConstData();
    const auto & currPort = input.ports[input.nodes[event.basedNode.value()].portId];

    for (auto itineraryId : event.relatedItineraryIds) {
        containersInPorts[currPort.id] += containersAssignedOnItineraries[itineraryId];
    }
}

void Evaluator::makePricingAction(const Event& event) {
    logAboutMakingPricingActionForEvent(event);

    assert(decisionManager);
    assert(actionManager);
    assert(decisionManager->getConstData().prices.size() > event.relativeTime);

    auto* action = actionManager->get();
    auto* decision = &decisionManager->getConstData();
    const auto& market = marketManager->getConstData();

    action->time = event.relativeTime;
    std::fill(std::begin(action->bookingsB[event.relativeTime]),
            std::end(action->bookingsB[event.relativeTime]), 0);

    auto wtopay = market.idEventToWpay.find(event.relativeTime);
    assert (wtopay != market.idEventToWpay.end());

    for (const auto& itinerary_price : decision->prices[event.relativeTime]) {
        auto buyers = wtopay->second.find(itinerary_price.first);
        if (buyers == wtopay->second.end())
            continue;
        auto price = itinerary_price.second;
        unsigned books = 0;
        unsigned shows = 0;
        for (const auto& spotShow : buyers->second) {
            if (spotShow.willingnessToPay >= price) {
                ++books;
                shows += spotShow.showFlag;
            }
        }
        action->bookingsB[event.relativeTime][itinerary_price.first] = books;
        totalBookings[itinerary_price.first] += books;
        action->spotMarketDemandN[itinerary_price.first] += shows;

        assert(books >= 0);
        assert(price > 0);
        statistics.spotProfit += books * price;

        logPricingBookingsRevenue(itinerary_price.first, price, books);
        statistics.sumSpotBookingAmount += books;
    }
}

void Evaluator::makeCutoffAction(const InputData::Event& event) {

    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Evaluator: making cut-off action on time " <<  event.relativeTime << "\n";

    const auto& inputLinks = config.linksManager->getConstData();
    const auto& input = config.inputManager->getConstData();
    auto* action = actionManager->get();
    auto* decision = &decisionManager->getConstData();
    action->time = event.relativeTime;

    // demand realization already assigned
    for (unsigned itId : event.relatedItineraryIds) {

        stream << "Processing itinerary with id " << itId << "\n";

        assert(itId == input.itineraries[itId].id);
        assert(totalBookings[itId] >= action->spotMarketDemandN[itId]);
        auto diff = totalBookings[itId] - action->spotMarketDemandN[itId];

        stream << "Bookings = " << totalBookings[itId] << "\n";
        stream << "Demand = " << action->spotMarketDemandN[itId] << "\n";
        stream << "Removing return price from spot profit: "
            << input.itineraries[itId].returnPrice * diff << "\n";

        statistics.spotProfit -= input.itineraries[itId].returnPrice * diff;
        assert(input.itineraries[itId].returnPrice >= 0);
        assert(diff >= 0);

        stream << "Now processing allotments. \n";
        for (auto contractId : inputLinks.allotmentsWithItinerary[itId]) {
            if (!decision->allotmentAccepted[contractId])
                continue;

            auto place = inputLinks.allotmentItineraryToPlace.at(contractId).at(itId);
            auto showAmountN = action->allotmentDemandN[contractId][place].second;
            stream << "Shown amount = " << showAmountN << "\n";

            auto entryId = inputLinks.allotmentItineraryToEntry.at(contractId).at(itId);
            const auto & entry = input.allotmentEntries[entryId];
            assert (entry.productAmount >= showAmountN);

            stream << "Inital negotiated amount = " << entry.productAmount << "\n";

            statistics.sumAllotmentBookingAmount += entry.productAmount;
            assert(entry.productAmount >= showAmountN);
            statistics.allotmentProfit += entry.cancellationPrice *
                (entry.productAmount - showAmountN);

            stream << "Adding cancellation payment to allotments: " << entry.cancellationPrice *
                (entry.productAmount - showAmountN) << "\n";
        }
    }
    stream << "Evaluator: current spot profit = " << statistics.spotProfit << "\n";
    stream << "Evaluator: cut-off action is made." << "\n";
}

void Evaluator::processAllotmentDecision() {
    const auto& input = config.inputManager->getConstData();
    const auto& inputLinks = config.linksManager->getConstData();
    const auto& market = marketManager->getConstData();
    auto* decision = &decisionManager->getConstData();
    auto* action = actionManager->get();

    selectedAllotments = decision->allotmentAccepted;

    for (const auto& group : input.allotmentGroups) {
        unsigned countValueOn = 0;
        for (const unsigned allotmentId : group) {
            if (decision->allotmentAccepted[allotmentId]) {
                ++countValueOn;
            }
        }
        assert(countValueOn <= 1);
    }
    for (unsigned i = 0; i < decision->allotmentAccepted.size(); ++i) {
        if (!decision->allotmentAccepted[i])
            continue;
        for (const auto & p : market.allotmentShowCount[i]) {
            auto place = inputLinks.allotmentItineraryToPlace.at(i).at(p.first);
            action->allotmentDemandN[i][place] = p;
        }
    }
}

void Evaluator::processCutoffDecision(const Event& event) {

    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Evaluator: processCutoffDecision for event "
        << event.relativeTime << "\n";

    const auto& input = config.inputManager->getConstData();
    const auto& inputLinks = config.linksManager->getConstData();
    auto* decision = &decisionManager->getConstData();
    auto* action = actionManager->get();

    assert(event.type == EventType::cutoff);
    const auto & basedArc = input.arcs[event.basedArc.value()];
    const auto & port = input.ports[input.nodes[basedArc.fromNode].portId];

    // pay for hiring containers
    statistics.containerProfit -= decision->hiredY[basedArc.id] * port.hiringCost;
    stream << "hired " << decision->hiredY[basedArc.id] << " containers, "
        << "paid " << decision->hiredY[basedArc.id] * port.hiringCost
        << "\n";
    containersInPorts[port.id] += decision->hiredY[basedArc.id];

    // process spot market
    for (auto itineraryId : event.relatedItineraryIds) {
        const auto& itinerary = input.itineraries[itineraryId];
        auto takenContainers = decision->nonEmptyContainersQ[itineraryId];
        stream << "spot market, itinerary " << itineraryId << "\n";
        stream << "spot market, taken Q_r = " << takenContainers << "\n";

        statistics.spotContainerCount += takenContainers;
        auto emptyContainers = decision->emptyContainersZ[itineraryId];

        stream << "spot market, taken Z_r = " << emptyContainers << "\n";

        statistics.emptyContainerCount += emptyContainers;

        assert(action->spotMarketDemandN[itineraryId] >= takenContainers);
        assert(takenContainers >= 0);
        assert(action->spotMarketDemandN[itineraryId] >= 0);
        double diff = double(action->spotMarketDemandN[itineraryId]) - takenContainers;
        stream << "spot market, declined " << int(diff) << "containers" << "\n";
        assert(diff + 1e-8 >= 0);
        // pay for not taken products
        statistics.spotProfit -= itinerary.declineCost * diff;
        assert(itinerary.declineCost >= 0);
        stream << "spot market, paying decline cost = " << itinerary.declineCost * diff
            << " for diff = " << diff << "\n";
        // pay transfer cost for non-empty containers
        statistics.spotProfit -= takenContainers * itinerary.cost;
        assert(takenContainers >= 0);
        assert(itinerary.cost >= 0);
        stream << "spot market, paying transfer cost = " <<
            takenContainers * itinerary.cost << "\n";
        // pay transfer cost for empty containers
        statistics.emptyContainerProfit -= emptyContainers * itinerary.emptyCost;
        stream << "spot market, paying transfer cost for empty containers: " <<
            emptyContainers * itinerary.emptyCost << "\n";
        // pay for waiting in port until departure

        statistics.containerProfit -= port.storageCost * event.duration
            * (takenContainers + emptyContainers);

        stream << "containers, paying for waiting in port " <<
            port.storageCost * event.duration *
            (takenContainers + emptyContainers) << "\n";

        // update assigned containers
        assert(containersAssignedOnItineraries[itineraryId] == 0);
        containersAssignedOnItineraries[itineraryId] = takenContainers + emptyContainers;
        assert(containersInPorts[port.id] >= takenContainers + emptyContainers);
        containersInPorts[port.id] -= (takenContainers + emptyContainers);
    }

    //process allotments
    for (auto itId : event.relatedItineraryIds) {
        const auto& itinerary = input.itineraries[itId];

        for (auto contractId : inputLinks.allotmentsWithItinerary[itId]) {
            if (!decision->allotmentAccepted[contractId])
                continue;

            auto place = inputLinks.allotmentItineraryToPlace.at(contractId).at(itId);
            auto showAmountN = action->allotmentDemandN[contractId][place].second;
            auto takenContainersQ = decision->allotmentContainersQ[contractId][place].second;
            statistics.allotmentContainerCount += takenContainersQ;

            auto entryId = inputLinks.allotmentItineraryToEntry.at(contractId).at(itId);
            const auto & entry = input.allotmentEntries[entryId];

            statistics.allotmentProfit += takenContainersQ * entry.price;
            assert(takenContainersQ >= 0);
            assert(entry.price >= 0);
            assert(showAmountN >= takenContainersQ);
            statistics.allotmentProfit -= (showAmountN - takenContainersQ) * itinerary.declineCost;

            // pay transfer cost for non-empty containers
            assert(itinerary.cost >= 0);
            statistics.allotmentProfit -= takenContainersQ * itinerary.cost;
            // pay for waiting in port until departure
            assert(event.duration >= 0);
            assert(port.storageCost >= 0);
            statistics.allotmentProfit -= port.storageCost * event.duration * takenContainersQ;

            // update containers
            containersAssignedOnItineraries[itId] += takenContainersQ;
            assert(containersInPorts[port.id] >= takenContainersQ);
            containersInPorts[port.id] -= takenContainersQ;
        }
        for (unsigned idArc : itinerary.orderedArcs) {
            const auto& arc = input.arcs[idArc];
            usedCapacity[arc.id] += containersAssignedOnItineraries[itinerary.id];

            if (arc.type == InputData::Arc::Type::travel) {
                const auto& vessel = input.vessels[arc.vesselId.value()];
                assert(usedCapacity[arc.id] <= vessel.capacity);
            }
        }
    }
    stream << "Evaluator: current spot profit = " << statistics.spotProfit << "\n";
    stream << "Evaluator: processCutoffDecision finished\n";
}

void Evaluator::updateContainersInPorts(double durationToNextPeriod) {
    const auto& input = config.inputManager->getConstData();
    auto* decision = &decisionManager->getConstData();

    unsigned timeNow = decision->time;
    for (unsigned portId = 0; portId < decision->offHiredInPortS[timeNow].size(); ++portId) {
        assert(containersInPorts[portId] >= decision->offHiredInPortS[timeNow][portId]);
        containersInPorts[portId] -= decision->offHiredInPortS[timeNow][portId];

        statistics.containerProfit -= input.ports[portId].offHiringCost
            * decision->offHiredInPortS[timeNow][portId];
        statistics.containerProfit-= input.ports[portId].storageCost
            * durationToNextPeriod * containersInPorts[portId];
    }
}

void Evaluator::checkFinalContainersInPorts() {
    const auto& input = config.inputManager->getConstData();
    for (unsigned idx = 0; idx < input.ports.size(); ++idx) {
        if (containersInPorts[idx] != input.ports[idx].finalContainerCount) {
            int diff = int(containersInPorts[idx]) - int(input.ports[idx].finalContainerCount);
            if (diff > 0) {
                statistics.containerProfit-= input.ports[idx].offHiringCost * diff;
            } else {
                statistics.containerProfit+= input.ports[idx].hiringCost * diff;
            }
            containersInPorts[idx] -= diff;
        }
    }
}

}   // namespace sea

