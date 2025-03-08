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
    const auto& allotment = input.allotments[allotmentId];
    double expectedProfit = 0.;
    for (const unsigned idxEntry: allotment.entries) {
        const auto& entry = input.allotmentEntries[idxEntry];
        double shippingCost = computeUnitShippingCost(input, links, entry.itinerary);
        expectedProfit += entry.productAmount * (
            (entry.price - shippingCost) * entry.showRate.estimatedProba +
            entry.cancellationPrice * (1. - entry.showRate.estimatedProba)
        );
    }
    return expectedProfit;
}


ByUnitExpectedProfit::ByUnitExpectedProfit(const BaselineAllotmentConfig& config)
    : AbstractAllotmentSorter(config, "ByUnitExpectedProfit") {}

double ByUnitExpectedProfit::getAllotmentMetric(unsigned allotmentId) const {
    const auto& input = inputManager->getConstData();
    const auto& links = linksManager->getConstData();
    const auto& allotment = input.allotments[allotmentId];
    double expectedProfit = 0.;
    size_t nEntries = 0.;
    for (const unsigned idxEntry: allotment.entries) {
        const auto& entry = input.allotmentEntries[idxEntry];
        double shippingCost = computeUnitShippingCost(input, links, entry.itinerary);
        expectedProfit += entry.productAmount * (
            (entry.price - shippingCost) * entry.showRate.estimatedProba +
            entry.cancellationPrice * (1. - entry.showRate.estimatedProba)
        );
        nEntries += entry.productAmount;
    }
    return expectedProfit / nEntries;
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
            updateStatsAtAllotmentSelection(&stats, input, allotmentId);
            for (unsigned idxEntry = 0; idxEntry < allotment.entries.size(); ++idxEntry) {
                const auto& entry = input.allotmentEntries[allotment.entries[idxEntry]];
                double shippingCost = computeUnitShippingCost(input, links, entry.itinerary);
                totalExpectedProfit += entry.productAmount * (
                    (entry.price - shippingCost) * entry.showRate.estimatedProba +
                    entry.cancellationPrice * (1. - entry.showRate.estimatedProba)
                );
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
    sorters.push_back(std::make_unique<TrivialSorter>(config));
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
