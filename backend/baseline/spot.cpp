// Implements Spot Itinerary Sorters and Order Metrics.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2025

#include "spot.h"
#include "plan.h"
#include "api.h"
#include "../../logging/logging.h"

#include <algorithm>
#include <map>
#include <memory>
#include <tuple>
#include <sstream>
#include <ranges>
#include <random>

namespace sea {
namespace backend {
namespace spot {

using EventType=InputData::Event::Type;

AbstractSpotSorter::AbstractSpotSorter(
        const BaselineSpotConfig& config, const std::string& sorterName)
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


ByExpectedTotalProfit::ByExpectedTotalProfit(const BaselineSpotConfig& config)
    : AbstractSpotSorter(config, "ByExpectedTotalProfit") {}

double ByExpectedTotalProfit::getRouteMetric(
        const BaselineStats& stats, const Demand& demand, unsigned idxItinerary) const {
    const auto& input = inputManager->getConstData();
    const auto& route = input.itineraries[idxItinerary];
    ItineraryPlan plan = buildItineraryPlan(
            input, linksManager->getConstData(), stats, route, demand);
    return plan.expectedRevenue;
}


ByExpectedUnitProfit::ByExpectedUnitProfit(const BaselineSpotConfig& config)
    : AbstractSpotSorter(config, "ByExpectedUnitProfit") {}

double ByExpectedUnitProfit::getRouteMetric(
        const BaselineStats& stats, const Demand& demand, unsigned idxItinerary) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    const auto& route = input.itineraries[idxItinerary];
    ItineraryPlan plan = buildItineraryPlan(
            input, links, stats, route, demand);
    double shippingCost = computeUnitShippingCost(input, links, idxItinerary);
    return (plan.price - shippingCost) * (route.showRate.estimatedProba)
        + (plan.price - route.returnPrice) * (1. - route.showRate.estimatedProba);
}


ByExpectedCapacity::ByExpectedCapacity(const BaselineSpotConfig& config)
    : AbstractSpotSorter(config, "ByExpectedCapacity") {}

double ByExpectedCapacity::getRouteMetric(
        const BaselineStats& stats, const Demand& demand, unsigned idxItinerary) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    const auto& route = input.itineraries[idxItinerary];
    ItineraryPlan plan = buildItineraryPlan(
            input, links, stats, route, demand);
    return plan.demand * route.showRate.estimatedProba;
}


ByPrice::ByPrice(const BaselineSpotConfig& config)
    : AbstractSpotSorter(config, "ByPrice") {}

double ByPrice::getRouteMetric(
        const BaselineStats& stats, const Demand& demand, unsigned idxItinerary) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    const auto& route = input.itineraries[idxItinerary];
    ItineraryPlan plan = buildItineraryPlan(
            input, links, stats, route, demand);
    return plan.price;
}


ByShowRate::ByShowRate(const BaselineSpotConfig& config)
    : AbstractSpotSorter(config, "ByShowRate") {}

double ByShowRate::getRouteMetric(
        [[maybe_unused]] const BaselineStats& stats,
        [[maybe_unused]] const Demand& demand,
        unsigned idxItinerary) const {
    const auto& input = inputManager->getConstData();
    const auto& route = input.itineraries[idxItinerary];
    return route.showRate.estimatedProba;
}


ByOptimisticCapacity::ByOptimisticCapacity(const BaselineSpotConfig& config)
    : AbstractSpotSorter(config, "ByOptimisticCapacity") {}

double ByOptimisticCapacity::getRouteMetric(
        const BaselineStats& stats, const Demand& demand, unsigned idxItinerary) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    const auto& route = input.itineraries[idxItinerary];
    ItineraryPlan plan = buildItineraryPlan(
            input, links, stats, route, demand);
    return plan.demand;
}


ByOptimisticProfit::ByOptimisticProfit(const BaselineSpotConfig& config)
    : AbstractSpotSorter(config, "ByOptimisticProfit") {}

double ByOptimisticProfit::getRouteMetric(
        const BaselineStats& stats, const Demand& demand, unsigned idxItinerary) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    const auto& route = input.itineraries[idxItinerary];
    ItineraryPlan plan = buildItineraryPlan(
            input, links, stats, route, demand);
    if (plan.price == std::numeric_limits<double>::max()) {
        return 0.;
    }
    return plan.demand * (plan.price - computeUnitShippingCost(input, links, idxItinerary));
}


RandomSpotSorter::RandomSpotSorter(size_t randomSeed)
    : seed(randomSeed) {}

std::vector<unsigned> RandomSpotSorter::selectOrder(
        const InputData::Event& event, [[maybe_unused]] const BaselineStats& stats) const {
    assert(event.type == EventType::pricing);
    std::vector<unsigned> itineraryIds = event.relatedItineraryIds;
    std::default_random_engine generator(seed);
    generator.seed(seed);
    std::shuffle(itineraryIds.begin(), itineraryIds.end(), generator);
    return itineraryIds;
}

std::string RandomSpotSorter::getName() const {
    return "RandomSpotSorter:seed=" + std::to_string(seed);
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
        const BaselineSpotConfig& config)
        : inputManager(config.inputManager)
        , linksManager(config.linksManager)
        , metric(nullptr) {
    metric = std::make_unique<SpotEventProfitMetric>(config);
    sorters.push_back(std::make_unique<ByExpectedTotalProfit>(config));
    sorters.push_back(std::make_unique<ByExpectedUnitProfit>(config));
    sorters.push_back(std::make_unique<ByExpectedCapacity>(config));
    sorters.push_back(std::make_unique<ByPrice>(config));
    sorters.push_back(std::make_unique<ByShowRate>(config));
    sorters.push_back(std::make_unique<ByOptimisticCapacity>(config));
    sorters.push_back(std::make_unique<ByOptimisticProfit>(config));
    sorters.push_back(std::make_unique<TrivialItineraryOrder>());
    const size_t RANDOM_COUNT = 12;
    for (size_t seed = 1; seed <= RANDOM_COUNT; ++seed) {
        sorters.push_back(std::make_unique<RandomSpotSorter>(seed));
    }
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


SpotEventProfitMetric::SpotEventProfitMetric(const BaselineSpotConfig& config)
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
