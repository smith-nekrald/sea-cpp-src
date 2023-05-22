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

void Evaluator::logAllotmentDecision() {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << "Allotment decision: " << decisionManager->getConstData();
}

void Evaluator::logSpotProfitAfterLastEvent(const Event& event) {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << "After event: " << event.relativeTime
            << " spotProfit = " << statistics.spotProfit << "\n";
}

void Evaluator::logUsedCapacity() {
    logging::getOutTestLogger().debugStream() << "Final counts on all arcs: ";
    auto debugStream = logging::getOutTestLogger().getStream(log4cpp::Priority::DEBUG);

    const sea::InputData* inputPtr = config.inputManager->get();
    for (const auto& arc : inputPtr->arcs) {
        debugStream << usedCapacity[arc.id] << " ";
    }
    debugStream.flush();
}

void Evaluator::logProfits() {
    auto& logger = logging::getEvaluationLogger();
    logger.noticeStream() << "spotProfit = " << statistics.spotProfit;
    logger.noticeStream() << "allotmentProfit = " << statistics.allotmentProfit;
    logger.noticeStream() << "fullProfit = " << statistics.fullProfit;
}

void Evaluator::logAboutArrival() {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << " Arrival event. \n";
}

void Evaluator::logAboutCutoff() {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << " Cutoff event. \n";
}

void Evaluator::logAboutPricing() {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << " Pricing event. \n";
}

void Evaluator::logAboutOffhiring() {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << " Off-hiring. \n";
}


void Evaluator::logEvaluatorLaunched(const Event& event) {
    auto& logger = logging::getEvaluationLogger();
    logger.debugStream() << "Evaluator is launched for event: " << event.relativeTime << "\n";
}

void Evaluator::logPricingBookingsRevenue(unsigned itinerary, double price, unsigned bookings) {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Itinerary: " << itinerary << " Price: " << price << " ";
    stream << "Bookings: " << bookings << "\n";
    stream << "Adding to spot profit revenue = " << bookings * price << "\n";
}

void Evaluator::logAboutMakingPricingActionForEvent(const Event& event) {
    auto& logger = logging::getEvaluationLogger();
    auto stream = logger.getStream(log4cpp::Priority::DEBUG);
    stream << "Evaluator: Making pricing action for event " << event.relativeTime << "\n";
}


} // namespace sea
