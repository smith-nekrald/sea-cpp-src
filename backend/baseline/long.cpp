#include "long.h"
#include "api.h"
#include "baseline_stats.h"

#include <numeric>
#include <algorithm>

namespace sea {
namespace backend {

ByTotalExpectedProfit::ByTotalExpectedProfit(const AllotmentSorterConfig& config)
    : input(config.inputManager->getConstData())
    , links(config.linksManager->getConstData()) {}

double ByTotalExpectedProfit::getTotalExpectedProfit(unsigned allotmentId) const {
    const auto& allotment = input.allotments[allotmentId];
    double expectedProfit = 0.;
    for (const unsigned idxEntry: allotment.entries) {
        const auto& entry = input.allotmentEntries[idxEntry];
        expectedProfit += entry.productAmount * (
            entry.price * entry.showRate.estimatedProba +
            entry.cancellationPrice * (1. - entry.showRate.estimatedProba)
        );
    }
    return expectedProfit;
}

std::vector<unsigned> ByTotalExpectedProfit::selectOrder() {
    std::vector<unsigned> allotmentOrder(input.allotments.size());
    std::iota(std::begin(allotmentOrder), std::end(allotmentOrder), 0);
    std::sort(std::begin(allotmentOrder), std::end(allotmentOrder),
            [&](unsigned lhs, unsigned rhs) {
                return getTotalExpectedProfit(lhs) > getTotalExpectedProfit(rhs);
            });
    return allotmentOrder;
}

ByUnitExpectedProfit::ByUnitExpectedProfit(const AllotmentSorterConfig& config)
    : input(config.inputManager->getConstData())
    , links(config.linksManager->getConstData()) {}

double ByUnitExpectedProfit::getUnitExpectedProfit(unsigned allotmentId) const {
    const auto& allotment = input.allotments[allotmentId];
    double expectedProfit = 0.;
    size_t nEntries = 0.;
    for (const unsigned idxEntry: allotment.entries) {
        const auto& entry = input.allotmentEntries[idxEntry];
        expectedProfit += entry.productAmount * (
            entry.price * entry.showRate.estimatedProba +
            entry.cancellationPrice * (1. - entry.showRate.estimatedProba)
        );
        nEntries += entry.productAmount;
    }
    return expectedProfit / nEntries;
}

std::vector<unsigned> ByUnitExpectedProfit::selectOrder() {
    std::vector<unsigned> allotmentOrder(input.allotments.size());
    std::iota(std::begin(allotmentOrder), std::end(allotmentOrder), 0);
    std::sort(std::begin(allotmentOrder), std::end(allotmentOrder),
            [&](unsigned lhs, unsigned rhs) {
                return getUnitExpectedProfit(lhs) > getUnitExpectedProfit(rhs);
            });
    return allotmentOrder;
}

EstimatedProfitMetric::EstimatedProfitMetric(const AllotmentSorterConfig& config)
    : input(config.inputManager->getConstData())
    , links(config.linksManager->getConstData()) {}

double EstimatedProfitMetric::score(const std::vector<unsigned>& allotmentOrder) {
    BaselineStats stats;
    initBaselineStats(&stats, input);
    double totalExpectedProfit = 0.;
    for (const auto& allotmentId: allotmentOrder) {
        const auto& allotment = input.allotments[allotmentId];
        bool allotmentAvailable = checkIfAllotmentAvailable(input, stats, allotmentId);
        if (allotmentAvailable) {
            updateStatsAtAllotmentSelection(&stats, input, allotmentId);
            for (unsigned idxEntry = 0; idxEntry < allotment.entries.size(); ++idxEntry) {
                const auto& entry = input.allotmentEntries[allotment.entries[idxEntry]];
                totalExpectedProfit += entry.productAmount * (
                    entry.price * entry.showRate.estimatedProba +
                    entry.cancellationPrice * (1. - entry.showRate.estimatedProba)
                );
            }
        }
    }
    return totalExpectedProfit;
}

LongCompositeSorter::LongCompositeSorter(const AllotmentSorterConfig& config)
        : input(config.inputManager->getConstData())
        , links(config.linksManager->getConstData()) {
    metric = std::make_unique<EstimatedProfitMetric>(config);
    sorters.push_back(std::make_unique<ByTotalExpectedProfit>(config));
    sorters.push_back(std::make_unique<ByUnitExpectedProfit>(config));
}

std::vector<unsigned> LongCompositeSorter::selectOrder() {
    std::vector<unsigned> selectedOrder;
    std::iota(std::begin(selectedOrder), std::end(selectedOrder), 0);
    double selectedScore = metric->score(selectedOrder);
    for (const auto& orderer: sorters) {
        auto selectedOrder = orderer->selectOrder();
        auto reversedOrder = selectedOrder;
        std::reverse(std::begin(selectedOrder), std::end(selectedOrder));
        for (const auto& candidateOrder: {selectedOrder, reversedOrder}) {
            double candidateScore = metric->score(candidateOrder);
            if (candidateScore > selectedScore) {
                selectedScore = candidateScore;
                selectedOrder = candidateOrder;
            }
        }
    }
    return selectedOrder;
}

} // namespace sea
} // namespace backend
