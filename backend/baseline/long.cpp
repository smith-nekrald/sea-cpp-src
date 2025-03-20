// Implements baseline allotment sorters and allotment order metrics.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2025

#include "long.h"
#include "api.h"
#include "baseline_stats.h"
#include "../../logging/logging.h"
#include "../../input/input_data.h"

#include <numeric>
#include <algorithm>
#include <sstream>
#include <tuple>
#include <iterator>
#include <random>
#include <cassert>

namespace sea {
namespace backend {
namespace allotment {

AbstractAllotmentSorter::AbstractAllotmentSorter(
        const BaselineAllotmentConfig& config,
        const std::string& sorterName)
        : name(sorterName)
        , inputManager(config.inputManager)
        , linksManager(config.linksManager) {}

std::string AbstractAllotmentSorter::getName() const {
    return name;
}

std::vector<unsigned> AbstractAllotmentSorter::selectOrder() const {
    const auto& input = inputManager->getConstData();
    std::vector<double> metricValues;
    for (const auto& allotment : input.allotments) {
        metricValues.push_back(getAllotmentMetric(allotment.id));
    }
    logMetricValues(metricValues);
    std::vector<unsigned> allotmentOrder(input.allotments.size(), 0);
    std::iota(std::begin(allotmentOrder), std::end(allotmentOrder), 0);
    std::sort(std::begin(allotmentOrder), std::end(allotmentOrder),
            [&](unsigned lhs, unsigned rhs) {
                return metricValues[lhs] > metricValues[rhs];
            });
    return allotmentOrder;
}

void AbstractAllotmentSorter::logMetricValues(const std::vector<double>& metricValues) const {
    auto& logger = logging::getBackendLogger();
    logger.info("Allotment Sorter: " + getName());
    logger.info("Sort Metric Values Per Allotment Id");
    for (unsigned idx = 0; idx < metricValues.size(); ++idx) {
        logger.info("Allotment Id: " + std::to_string(idx)
                + " Metric Value: " + std::to_string(metricValues[idx]));
    }
}


RandomAllotmentSorter::RandomAllotmentSorter(size_t randomSeed, size_t nAllotments)
    : seed(randomSeed)
    , allotmentCount(nAllotments) {}

std::vector<unsigned> RandomAllotmentSorter::selectOrder() const {
    std::vector<unsigned> allotmentOrder(allotmentCount, 0);
    std::iota(std::begin(allotmentOrder), std::end(allotmentOrder), 0);
    std::default_random_engine generator(seed);
    generator.seed(seed);
    std::shuffle(std::begin(allotmentOrder), std::end(allotmentOrder), generator);
    return allotmentOrder;
}

std::string RandomAllotmentSorter::getName() const {
    return "RandomAllotmentSorter:seed=" + std::to_string(seed);
}


TrivialSorter::TrivialSorter(const BaselineAllotmentConfig& config)
    : inputManager(config.inputManager) {}

std::vector<unsigned> TrivialSorter::selectOrder() const {
    const auto& input = inputManager->getConstData();
    std::vector<unsigned> allotmentOrder(input.allotments.size(), 0);
    std::iota(std::begin(allotmentOrder), std::end(allotmentOrder), 0);
    return allotmentOrder;
}

std::string TrivialSorter::getName() const {
    return "TrivialSorter";
}


ByTotalExpectedProfit::ByTotalExpectedProfit(const BaselineAllotmentConfig& config)
    : AbstractAllotmentSorter(config, "ByTotalExpectedProfit") {}

double ByTotalExpectedProfit::getAllotmentMetric(unsigned allotmentId) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    return computeExpectedAllotmentProfit(input, links, allotmentId);
}


ByUnitExpectedProfit::ByUnitExpectedProfit(const BaselineAllotmentConfig& config)
    : AbstractAllotmentSorter(config, "ByUnitExpectedProfit") {}

double ByUnitExpectedProfit::getAllotmentMetric(unsigned allotmentId) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    const auto& allotment = input.allotments[allotmentId];
    double expectedProfit = computeExpectedAllotmentProfit(input, links, allotmentId);
    size_t nEntries = 0.;
    for (const unsigned idxEntry: allotment.entries) {
        const auto& entry = input.allotmentEntries[idxEntry];
        nEntries += entry.productAmount;
    }
    return expectedProfit / nEntries;
}


ByTotalExpectedCapacity::ByTotalExpectedCapacity(const BaselineAllotmentConfig& config)
    : AbstractAllotmentSorter(config, "ByTotalExpectedCapacity") {}

double ByTotalExpectedCapacity::getAllotmentMetric(unsigned allotmentId) const {
    const auto& input = inputManager->getConstData();
    const auto& allotment = input.allotments[allotmentId];
    double expectedCapacity = 0.;
    for (unsigned idxEntry = 0; idxEntry < allotment.entries.size(); ++idxEntry) {
        const auto& entry = input.allotmentEntries[allotment.entries[idxEntry]];
        expectedCapacity += entry.productAmount * entry.showRate.estimatedProba;
    }
    return expectedCapacity;
}


ByTotalOptimisticCapacity::ByTotalOptimisticCapacity(const BaselineAllotmentConfig& config)
    : AbstractAllotmentSorter(config, "ByTotalOptimisticCapacity") {}

double ByTotalOptimisticCapacity::getAllotmentMetric(unsigned allotmentId) const {
    const auto& input = inputManager->getConstData();
    const auto& allotment = input.allotments[allotmentId];
    double optimisticCapacity = 0.;
    for (unsigned idxEntry = 0; idxEntry < allotment.entries.size(); ++idxEntry) {
        const auto& entry = input.allotmentEntries[allotment.entries[idxEntry]];
        optimisticCapacity += entry.productAmount;
    }
    return optimisticCapacity;
}


ByWeightedShowRate::ByWeightedShowRate(const BaselineAllotmentConfig& config)
    : AbstractAllotmentSorter(config, "ByWeightedShowRate") {}

double ByWeightedShowRate::getAllotmentMetric(unsigned allotmentId) const {
    const auto& input = inputManager->getConstData();
    const auto& allotment = input.allotments[allotmentId];
    double weightedShowRate = 0.;
    size_t nEntries = 0;
    for (unsigned idxEntry = 0; idxEntry < allotment.entries.size(); ++idxEntry) {
        const auto& entry = input.allotmentEntries[allotment.entries[idxEntry]];
        weightedShowRate += entry.productAmount * entry.showRate.estimatedProba;
        nEntries += entry.productAmount;
    }
    weightedShowRate /= nEntries;
    return weightedShowRate;
}


ByWeightedPrice::ByWeightedPrice(const BaselineAllotmentConfig& config)
    : AbstractAllotmentSorter(config, "ByWeightedPrice") {}

double ByWeightedPrice::getAllotmentMetric(unsigned allotmentId) const {
    const auto& input = inputManager->getConstData();
    const auto& allotment = input.allotments[allotmentId];
    double weightedPrice = 0.;
    size_t nEntries = 0;
    for (unsigned idxEntry = 0; idxEntry < allotment.entries.size(); ++idxEntry) {
        const auto& entry = input.allotmentEntries[allotment.entries[idxEntry]];
        weightedPrice += entry.price * entry.productAmount;
        nEntries += entry.productAmount;
    }
    weightedPrice /= nEntries;
    return weightedPrice;
}


ByTotalOptimisticProfit::ByTotalOptimisticProfit(const BaselineAllotmentConfig& config)
    : AbstractAllotmentSorter(config, "ByTotalOptimisticProfit") {}

double ByTotalOptimisticProfit::getAllotmentMetric(unsigned allotmentId) const {
    const auto& input = inputManager->getConstData();
    const auto& allotment = input.allotments[allotmentId];
    double optimisticProfit = 0.;
    for (unsigned idxEntry = 0; idxEntry < allotment.entries.size(); ++idxEntry) {
        const auto& entry = input.allotmentEntries[allotment.entries[idxEntry]];
        optimisticProfit += entry.productAmount * entry.price;
    }
    return optimisticProfit;
}


EstimatedProfitMetric::EstimatedProfitMetric(const BaselineAllotmentConfig& config)
    : inputManager(config.inputManager)
    , linksManager(config.linksManager) {}

double EstimatedProfitMetric::score(const std::vector<unsigned>& allotmentOrder) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    BaselineStats stats;
    initBaselineStats(&stats, input);
    double totalExpectedProfit = 0.;
    for (const auto& allotmentId: allotmentOrder) {
        const auto& allotment = input.allotments[allotmentId];
        assert(allotment.id == allotmentId);
        bool allotmentAvailable = checkIfAllotmentAvailable(input, stats, allotmentId);
        if (allotmentAvailable) {
            double allotmentProfit = computeExpectedAllotmentProfit(input, links, allotmentId);
            if (allotmentProfit > 0.) {
                updateStatsAtAllotmentSelection(&stats, input, allotmentId);
                totalExpectedProfit += allotmentProfit;
            }
        }
    }
    return totalExpectedProfit;
}


LongCompositeSorter::LongCompositeSorter(const BaselineAllotmentConfig& config)
        : inputManager(config.inputManager)
        , linksManager(config.linksManager)
        , metric(nullptr) {
    metric = std::make_unique<EstimatedProfitMetric>(config);
    sorters.push_back(std::make_unique<ByTotalExpectedProfit>(config));
    sorters.push_back(std::make_unique<ByUnitExpectedProfit>(config));
    sorters.push_back(std::make_unique<ByTotalExpectedCapacity>(config));
    sorters.push_back(std::make_unique<ByTotalOptimisticCapacity>(config));
    sorters.push_back(std::make_unique<ByWeightedShowRate>(config));
    sorters.push_back(std::make_unique<ByWeightedPrice>(config));
    sorters.push_back(std::make_unique<ByTotalOptimisticProfit>(config));
    sorters.push_back(std::make_unique<TrivialSorter>(config));
    const auto& input = config.inputManager->getConstData();
    const size_t RANDOM_COUNT = 12;
    for (size_t seed = 1; seed <= RANDOM_COUNT; ++seed) {
        sorters.push_back(
            std::make_unique<RandomAllotmentSorter>(seed, input.allotments.size()));
    }
}

std::vector<unsigned> LongCompositeSorter::selectOrder() const {
    const auto& input = inputManager->getConstData();
    std::vector<unsigned> selectedOrder(input.allotments.size(), 0) ;
    std::iota(std::begin(selectedOrder), std::end(selectedOrder), 0);
    double selectedScore = metric->score(selectedOrder);

    for (const auto& orderer: sorters) {
        auto providedOrder = orderer->selectOrder();
        auto reversedOrder = providedOrder;
        std::reverse(std::begin(providedOrder), std::end(providedOrder));
        for (bool reverseFlag: {true, false}) {
            auto candidateOrder = reverseFlag ? reversedOrder : providedOrder;
            double candidateScore = metric->score(candidateOrder);
            logSortScore(candidateOrder, orderer->getName(), reverseFlag, candidateScore);
            if (candidateScore > selectedScore) {
                std::tie(selectedScore, selectedOrder) = std::tie(candidateScore, candidateOrder);
            }
        }
    }
    logSortScore(selectedOrder, getName(), false, selectedScore);
    return selectedOrder;
}

void LongCompositeSorter::logSortScore(
        const std::vector<unsigned>& allotmentOrder,
        const std::string& sorterName,
        bool reverseOrder,
        double score) const {
    auto& logger = logging::getBackendLogger();
    std::ostringstream stream;
    std::copy(allotmentOrder.begin(), allotmentOrder.end(),
            std::ostream_iterator<unsigned>(stream, " "));
    logger.info(
            "Allotment Sequence: " + stream.str()
            + " Sorter: " + sorterName
            + " Reverse Order: " + std::to_string(reverseOrder)
            + " Sort Score: " + std::to_string(score));
}

std::string LongCompositeSorter::getName() const {
    return "Long Composite Sorter";
}

} // namespace allotment
} // namespace backend
} // namespace sea
