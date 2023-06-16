// Implementation for Evaluator and Launcher Logging.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "evaluator.h"
#include "launcher.h"
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

void Evaluator::logAllotmentDecision() const {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << "Allotment decision: " << decisionManager->getConstData();
}

void Evaluator::logSpotProfitAfterLastEvent(const Event& event) const {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << "After event: " << event.relativeTime
            << " spotProfit = " << statistics.spotProfit << "\n";
}

void Evaluator::logUsedCapacity() const {
    logging::getOutTestLogger().debugStream() << "Final counts on all arcs: ";
    auto debugStream = logging::getOutTestLogger().getStream(log4cpp::Priority::DEBUG);

    const sea::InputData* inputPtr = config.inputManager->get();
    for (const auto& arc : inputPtr->arcs) {
        debugStream << usedCapacity[arc.id] << " ";
    }
    debugStream.flush();
}

void Evaluator::logProfits() const {
    auto& logger = logging::getEvaluationLogger();
    logger.noticeStream() << "spotProfit = " << statistics.spotProfit;
    logger.noticeStream() << "allotmentProfit = " << statistics.allotmentProfit;
    logger.noticeStream() << "fullProfit = " << statistics.fullProfit;
}

void Evaluator::logAboutArrival() const {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << " Arrival event. \n";
}

void Evaluator::logAboutCutoff() const {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << " Cutoff event. \n";
}

void Evaluator::logAboutPricing() const {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << " Pricing event. \n";
}

void Evaluator::logAboutOffhiring() const {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << " Off-hiring. \n";
}


void Evaluator::logEvaluatorLaunched(const Event& event) const {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << "Evaluator is launched for event: " << event.relativeTime << "\n";
}

void Evaluator::logPricingBookingsRevenue(
        unsigned itinerary, double price, unsigned bookings) const {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Itinerary: " << itinerary << " Price: " << price << " ";
    stream << "Bookings: " << bookings << "\n";
    stream << "Adding to spot profit revenue = " << bookings * price << "\n";
}

void Evaluator::logAboutMakingPricingActionForEvent(const Event& event) const {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Evaluator: Making pricing action for event " << event.relativeTime << "\n";
}

void Evaluator::logStartMakeCutoffAction(const InputData::Event& event) const {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Evaluator: making cut-off action on time " <<  event.relativeTime << "\n";
}

void Evaluator::logItineraryInfoInMakeCutoffAction(unsigned itineraryId) const {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    auto* action = actionManager->get();
    stream << "Processing itinerary with id " << itineraryId << "\n";
    stream << "Bookings = " << totalBookings[itineraryId] << "\n";
    stream << "Demand = " << action->spotMarketDemandN[itineraryId] << "\n";
    stream << "Next processing allotments. \n";
}

void Evaluator::logAllotmentInfoInMakeCutoffAction(
        unsigned showAmountN, unsigned productAmount) const {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Shown amount = " << showAmountN << "\n";
    stream << "Inital negotiated amount = " << productAmount << "\n";
}

void Evaluator::logFinishedMakeCutoffAction() const {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Evaluator: current spot profit = " << statistics.spotProfit << "\n";
    stream << "Evaluator: cut-off action is made." << "\n";
}

void Evaluator::logStartProcessCutoffDecision(
        const InputData::Event& event, unsigned hiredAmount, double paidForHiring) const {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Evaluator: processCutoffDecision for event " << event.relativeTime << "\n";
    stream << "hired " << hiredAmount << " containers, " << "paid " << paidForHiring << "\n";
}

void Evaluator::logItinerarySpotInProcessCutoffDecision(
        unsigned itineraryId, unsigned shippedContainers, unsigned emptyContainers,
        double diffDemandShipped, double paidDeclineCost, double nonEmptyTransferCost,
        double emptyTransferCost,  double waitingCost) const {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "spot market, itinerary " << itineraryId << "\n";
    stream << "spot market, shipped Q_r = " << shippedContainers << "\n";
    stream << "spot market, shipped Z_r = " << emptyContainers << "\n";
    stream << "spot market, declined " << diffDemandShipped << "containers" << "\n";
    stream << "spot market, paying decline cost = " << paidDeclineCost
            << " for diff = " << diffDemandShipped << "\n";
    stream << "spot market, paying transfer cost = " << nonEmptyTransferCost << "\n";
    stream << "spot market, paying transfer cost for empty containers: "
            << emptyTransferCost  << "\n";
    stream << "containers, paying for waiting in port " << waitingCost << "\n";
};

void Evaluator::logFinishedProcessCutoffDecision() const {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Evaluator: current spot profit = " << statistics.spotProfit << "\n";
    stream << "Evaluator: processCutoffDecision finished\n";
}

void Launcher::logLauncherInitialization() const {
    logging::getEvaluationLogger().debug("Initizalized Launcher.");
}

void Launcher::logFinishedEvaluation() const {
    logging::getOutTestLogger().debugStream() << "Evaluation Finished.";
}

void Launcher::logStartedEvaluation(std::string algoName, unsigned idxMarket) const {
    logging::getEvaluationLogger().info("Started evaluation for algorithm: " + algoName);
    logging::getEvaluationLogger().info("Evaluation set index is: " + std::to_string(idxMarket));
    logging::getOutTestLogger().debugStream() << "Evaluation Started: "
                << algoName << " " << " market_idx= " << idxMarket;
}

void Launcher::logReadMarketDataset(const std::string& dataPath) const {
    logging::getRootLogger().info("Finished reading market for dataPath: " + dataPath);
}


} // namespace sea
