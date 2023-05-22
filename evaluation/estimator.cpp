// Implements Estimator methods. 

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "estimator.h"

namespace sea {

std::ostream& operator<<(std::ostream& out, const RevenueEstimate& estimate) {
    out << "revenue_estimation = " <<  (estimate.allotment + estimate.spotMarket)
        << " [spotMarket: " << estimate.spotMarket << " + allotments: "
        << estimate.allotment << "]" << std::endl;
    return out;
}

RevenueEstimate BaseEstimator::calcUpperBound(
        const InputData& input, const InputLinks& links, const MarketData& market) {
    RevenueEstimate estimate;
    assert(estimate.allotment == 0 && estimate.spotMarket == 0);
    estimate.allotment = estimateAllotments(input, links, market);
    estimate.spotMarket = estimateSpotMarket(input, links, market);
    return estimate;
}

void BaseEstimator::estimateEachAllotment(
        const InputData& input, const InputLinks& links, const MarketData& market) {
    allotmentProfitEstimation.resize(input.allotments.size());
    assert(market.allotmentShowCount.size() == input.allotments.size());
    for (unsigned contractId = 0; contractId < input.allotments.size(); ++contractId) {
        double possibleRevenue = 0;
        // Possible revenue is a sum of possible revenues along allotment entries.
        for (const auto & point: market.allotmentShowCount[contractId]) {
            unsigned showAmount = point.second;
            const auto & itinerary = input.itineraries[point.first];
            unsigned entryId = links.allotmentItineraryToEntry.at(contractId).at(itinerary.id);
            const auto & entry = input.allotmentEntries[entryId];
            // Those who arrive may generate price - cost, those who no-show pay cancellation fee.
            double revenueDelta = 0.;
            revenueDelta += std::max<double>(0., entry.price - itinerary.cost) * showAmount;
            assert(showAmount <= entry.productAmount);
            revenueDelta += (entry.productAmount - showAmount) * entry.cancellationPrice;
            possibleRevenue += std::max<double>(revenueDelta, 0.);
        }
        allotmentProfitEstimation[contractId] = std::max<double>(0, possibleRevenue);
    }
}

double BaseEstimator::estimateAllotments(
        const InputData& input, const InputLinks& links, const MarketData& market) {
    estimateEachAllotment(input, links, market);
    double simpleSum = 0.;
    for (const auto& item : allotmentProfitEstimation) {
        simpleSum += item;
    }
    return simpleSum;
}

double BaseEstimator::estimateSpotMarket(
        const InputData& input, const InputLinks&, const MarketData& market) {
    double estimate = 0;
    for (const auto & entry: market.idEventToWpay) {
        for (const auto & pairSpot : entry.second) {
            const auto & itinerary = input.itineraries[pairSpot.first];
            assert(itinerary.declineCost > itinerary.cost);
            for (const auto & spotShow : pairSpot.second) {
                // Each shown may generate at most WTP - cost. Each no-shown 
                // client may generate at most WTP - return price.
                if (spotShow.showFlag) {
                    estimate += std::max<double>(0, spotShow.willingnessToPay - itinerary.cost);
                } else {
                    estimate += std::max<double>(0,
                            spotShow.willingnessToPay - itinerary.returnPrice);
                }
            }
        }
    }
    return estimate;
}

double SmartEstimator::estimateSpotMarket(
        const InputData& input, const InputLinks& links, const MarketData& market) {
    double estimate = 0;
    for (const auto & entry : market.idEventToWpay) {
        for (const auto & pairSpot : entry.second) {
            // Sort by willingness to pay.
            auto spotShows = pairSpot.second;
            std::sort(spotShows.begin(), spotShows.end(),
                [](const SpotShow& lhs, const SpotShow& rhs) {
                    return lhs.willingnessToPay < rhs.willingnessToPay;
            });
            // Compute total amount who shows.
            unsigned showSum = 0;
            for (unsigned idx = 0; idx < spotShows.size(); ++idx) {
                if (spotShows[idx].showFlag) {
                    showSum += 1;
                }
            }
            // Iterate over price set to some willingness to pay. All 
            // with greater WTP pay this price and either show or no-show.
            double maxRevenue = 0;
            const auto & itinerary = input.itineraries[pairSpot.first];
            for (unsigned ind = 0; ind < spotShows.size(); ++ind) {
                unsigned customerCount = spotShows.size() - ind;
                unsigned noShowCount = customerCount - showSum;
                auto valueOnShow = std::max<double>(0,
                    spotShows[ind].willingnessToPay - itinerary.cost);
                auto valueOnCancel = std::max<double>(0,
                    spotShows[ind].willingnessToPay - itinerary.returnPrice);
                auto revenueNow = valueOnShow * showSum;
                revenueNow += valueOnCancel * noShowCount;
                if (maxRevenue < revenueNow) {
                    maxRevenue = revenueNow;
                }
                if (spotShows[ind].showFlag) {
                    showSum -= 1;
                }
            }
            estimate += maxRevenue;
        }
    }
    // Update with an estimate from Base Estimator.
    estimate = std::min<double>(estimate,
        BaseEstimator::estimateSpotMarket(input, links, market));
    return estimate;
}

double SmartEstimator::estimateAllotments(
        const InputData& input, const InputLinks& links, const MarketData& market) {
    estimateEachAllotment(input, links, market);

    // From each group, at most one allotment is selected.
    vector<bool> seenInGroups(input.allotments.size(), false);
    double groupEstimate = 0;
    for (const auto& group:  input.allotmentGroups) {
        double best = 0.;
        for (const auto& item : group) {
            best = std::max<double>(best, allotmentProfitEstimation[item]);
            seenInGroups[item] = true;
        }
        groupEstimate += best;
    }

    // Accounts for those allotments that are not present in any group.
    for (std::size_t idx = 0; idx < input.allotments.size(); ++idx) {
        if (!seenInGroups[idx]) {
            groupEstimate += allotmentProfitEstimation[idx];
            seenInGroups[idx] = true;
        }
    }

    // Updates with an estimate from Base Estimator.
    double estimate = std::min(groupEstimate,
            BaseEstimator::estimateAllotments(input, links, market));
    return estimate;
}

}   // namespace sea

