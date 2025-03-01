#include "long.h"
#include "api.h"
#include "baseline_stats.h"

#include <numeric>
#include <algorithm>

namespace sea {
namespace backend {
namespace allotment {

AbstractAllotmentSorter::AbstractAllotmentSorter(const AllotmentSorterConfig& config)
    : inputManager(config.inputManager)
    , linksManager(config.linksManager) {}

std::vector<unsigned> AbstractAllotmentSorter::selectOrder() const {
    const auto& input = inputManager->getConstData();
    std::vector<unsigned> allotmentOrder(input.allotments.size(), 0);
    std::iota(std::begin(allotmentOrder), std::end(allotmentOrder), 0);
    std::sort(std::begin(allotmentOrder), std::end(allotmentOrder),
            [&](unsigned lhs, unsigned rhs) {
                return getAllotmentMetric(lhs) > getAllotmentMetric(rhs);
            });
    return allotmentOrder;
}


ByTotalExpectedProfit::ByTotalExpectedProfit(const AllotmentSorterConfig& config)
    : AbstractAllotmentSorter(config) {}

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


ByUnitExpectedProfit::ByUnitExpectedProfit(const AllotmentSorterConfig& config)
    : AbstractAllotmentSorter(config) {}

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


EstimatedProfitMetric::EstimatedProfitMetric(const AllotmentSorterConfig& config)
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


LongCompositeSorter::LongCompositeSorter(const AllotmentSorterConfig& config)
        : inputManager(config.inputManager)
        , linksManager(config.linksManager)
        , metric(nullptr) {
    metric = std::make_unique<EstimatedProfitMetric>(config);
    sorters.push_back(std::make_unique<ByTotalExpectedProfit>(config));
    sorters.push_back(std::make_unique<ByUnitExpectedProfit>(config));
}

std::vector<unsigned> LongCompositeSorter::selectOrder() const {
    const auto& input = inputManager->getConstData();
    std::vector<unsigned> selectedOrder(input.allotments.size(), 0) ;
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

} // namespace allotment
} // namespace backend
} // namespace sea
