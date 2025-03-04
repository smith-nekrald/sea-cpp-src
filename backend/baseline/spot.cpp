#include "spot.h"
#include "plan.h"
#include "../../logging/logging.h"

#include <algorithm>
#include <map>
#include <tuple>
#include <sstream>
#include <ranges>

namespace sea {
namespace backend {
namespace spot {

using EventType=InputData::Event::Type;

AbstractSpotSorter::AbstractSpotSorter(
        const SpotSorterConfig& config, const std::string& sorterName)
    : name(sorterName)
    , inputManager(config.inputManager)
    , linksManager(config.linksManager) {}

std::vector<unsigned> AbstractSpotSorter::selectOrder(
        const InputData::Event& event, const BaselineStats& stats) const {
    assert(event.type == EventType::pricing);
    std::vector<unsigned> itineraryIds = event.relatedItineraryIds;
    std::vector<Demand> demands = event.demands;
    std::map<unsigned, double> metricValues;
    for (const auto& [routeIdx, demand] : std::views::zip(itineraryIds, demands)) {
        metricValues[routeIdx] = getRouteMetric(stats, demand, routeIdx);
    }
    logMetricValues(event, metricValues);
    std::sort(std::begin(itineraryIds), std::end(itineraryIds),
            [&](unsigned lhs, unsigned rhs) {
                return metricValues[lhs] > metricValues[rhs];
            });
    return itineraryIds;
}

void AbstractSpotSorter::logMetricValues(
        const InputData::Event& event, const std::map<unsigned, double>& metricValues) const {
    auto& logger = logging::getBackendLogger();
    std::string message = "Metric Values. Spot Sorter: " + getName() + ". ";
    message += "Event: " + std::to_string(event.relativeTime) + ". \n";
    for (const auto& [routeIdx, metricValue] : metricValues) {
        message += "Itinerary " + std::to_string(routeIdx) + ": "
            + std::to_string(metricValue) + "\n";
    }
    message.pop_back();
    logger.info(message);
}

std::string AbstractSpotSorter::getName() const {
    return name;
}


ByExpectedTotalProfit::ByExpectedTotalProfit(const SpotSorterConfig& config)
    : AbstractSpotSorter(config, "ByExpectedTotalProfit") {}

double ByExpectedTotalProfit::getRouteMetric(
        const BaselineStats& stats, Demand& demand, unsigned idxItinerary) const {
    const auto& input = inputManager->getConstData();
    const auto& route = input.itineraries[idxItinerary];
    ItineraryPlan plan = buildItineraryPlan(
            inputManager->getConstData(),
            linksManager->getConstData(),
            stats, route, demand);
    return plan.expectedRevenue;
}

TrivialItineraryOrder::TrivialItineraryOrder() {};

std::vector<unsigned> TrivialItineraryOrder::selectOrder(
        const InputData::Event& event, [[maybe_unused]] const BaselineStats& stats) const {
    assert(event.type == EventType::pricing);
    return event.relatedItineraryIds;
}

std::string TrivialItineraryOrder::getName() const {
    return "TrivialItineraryOrder";
}


CompositeSpotSorter::CompositeSpotSorter(
        const SpotSorterConfig& config)
        : inputManager(config.inputManager)
        , linksManager(config.linksManager)
        , metric(nullptr) {
    metric = std::make_unique<SpotEventProfitMetric>(config);
    sorters.push_back(std::make_unique<ByExpectedTotalProfit>(config));
    sorters.push_back(std::make_unique<TrivialItineraryOrder>());
}

std::vector<unsigned> CompositeSpotSorter::selectOrder(
        const InputData::Event& event, const BaselineStats& stats) const {
    std::vector<unsigned> selectedIds = event.relatedItineraryIds;
    double selectedScore = metric->score(selectedIds, event, stats);
    for (const auto& orderer : sorters) {
        auto suggestedOrder = orderer->selectOrder(event, stats);
        for (auto reverseFlag : {false, true}) {
            auto candidateOrder = suggestedOrder;
            if (reverseFlag) {
                std::reverse(std::begin(candidateOrder), std::end(candidateOrder));
            }
            double candidateScore = metric->score(candidateOrder, event, stats);
            logScorePerOrder(orderer->getName(), candidateOrder, reverseFlag, candidateScore);
            if (candidateScore > selectedScore) {
                std::tie(selectedIds, selectedScore) = std::tie(candidateOrder, candidateScore);
            }
        }
    }
    return selectedIds;
}

std::string CompositeSpotSorter::getName() const {
    return "CompositeSpotSorter";
}

void CompositeSpotSorter::logScorePerOrder(
        const std::string& sorterName,
        const std::vector<unsigned>& spotOrder,
        bool reverseOrder,
        double score) const {
    std::ostringstream stream;
    std::copy(spotOrder.begin(), spotOrder.end(),
            std::ostream_iterator<unsigned>(stream, " "));
    auto& logger = logging::getBackendLogger();
    std::string message = "Total Order Score. Sorter Name : " + sorterName + ". ";
    message += "Itinerary Sequence: " + stream.str() + ". ";
    message += "Reverse Order: " + std::to_string(reverseOrder) + ". ";
    message += "Score: " + std::to_string(score) + ". ";
    logger.info(message);
}


SpotEventProfitMetric::SpotEventProfitMetric(const SpotSorterConfig& config)
    : inputManager(config.inputManager)
    , linksManager(config.linksManager) {}

double SpotEventProfitMetric::score(
        const std::vector<unsigned>& spotOrder,
        const InputData::Event& event,
        const BaselineStats& stats) const {
    auto localStats = stats;
    std::map<unsigned, unsigned> itineraryIdToIndex;
    for (auto [index, idxRoute] : std::views::enumerate(event.relatedItineraryIds)) {
        itineraryIdToIndex[idxRoute] = index;
    }
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    double expectedProfit = 0.;
    for (const auto& idxRoute : spotOrder) {
        unsigned idx = itineraryIdToIndex[idxRoute];
        auto demand = event.demands[idx];
        const InputData::Itinerary& route = input.itineraries[idxRoute];
        ItineraryPlan optimalPlan = backend::spot::buildItineraryPlan(
                input, links, localStats, route, demand);
        expectedProfit += optimalPlan.expectedRevenue;
        double expectedAmount = optimalPlan.demand * route.showRate.estimatedProba;
        for (unsigned idxArc : route.orderedArcs) {
            localStats.availableArcCapacity[idxArc] -= expectedAmount;
        }
        localStats.allocatedSpotRouteCapacity[route.id] += expectedAmount;
    }
    return expectedProfit;
}


} // namespace spot
} // namespace backend
} // namespace sea
