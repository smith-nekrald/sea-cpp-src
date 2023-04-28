#include "estimator.h"

namespace sea {

std::ostream& operator<<(std::ostream& out, const RevenueEstimate& estimate) {
    out << "revenue_estimation = " <<  (estimate.allotment + estimate.spotMarket) << " [spotMarket: "
        << estimate.spotMarket << " + allotments: " << estimate.allotment << "]" << std::endl;
    return out;
}

double BaseEstimator::estimateAllotments(
    const InputData& input,
    const InputLinks& links,
    const MarketData& market) const {
    vector<double> allotmentProfitEstimation(input.allotments.size());
    vector<bool> seenInGroups(input.allotments.size(), false);
    assert(market.allotmentShowCount.size() == input.allotments.size());
    for (ui32 contractId = 0; contractId < input.allotments.size(); ++contractId) {
        double possibleRevenue = 0;

        for (const auto & p : market.allotmentShowCount[contractId]) {
            ui32 showAmount = p.second;
            const auto & itinerary = input.itineraries[p.first];
            ui32 entryId = links.allotmentItineraryToEntry.at(contractId).at(itinerary.id);
            const auto & entry = input.allotmentEntries[entryId];

            assert(showAmount <= entry.productAmount);
            double revenueDelta = 0.;
            revenueDelta += (entry.price - itinerary.cost) * showAmount;
            revenueDelta += (entry.productAmount - showAmount) * entry.cancellationPrice;
            possibleRevenue += std::max<double>(revenueDelta, 0.);
        }
        allotmentProfitEstimation[contractId] = std::max<double>(0, possibleRevenue);
    }

    double simpleSum = 0.;
    for(const auto& item : allotmentProfitEstimation) {
        simpleSum += item;
    }

    double eval = 0;
    for (const auto& group:  input.allotmentGroups) {
        double best = 0.;
        for (const auto& item : group) {
            best = std::max<double>(best, allotmentProfitEstimation[item]);
            seenInGroups[item] = true;
        }
        eval += best;
    }
    for (std::size_t idx = 0; idx < input.allotments.size(); ++idx) {
        if (!seenInGroups[idx]) {
            eval += allotmentProfitEstimation[idx];
            seenInGroups[idx] = true;
        }
    }
    eval = std::min(eval, simpleSum);
    return eval;
}

double BaseEstimator::estimateSpotMarket(
    const InputData& input,
    const InputLinks&,
    const MarketData& market) const {
    double eval = 0;
    for (const auto & e : market.idEventToWpay) {
        for (const auto & p : e.second) {
            const auto & itinerary = input.itineraries[p.first];
            for (const auto & spotShow : p.second) {
                if (spotShow.showFlag) {
                    assert(itinerary.declineCost > itinerary.cost);
                    eval += std::max<double>(0, spotShow.willingnessToPay - itinerary.cost);
                } else {
                    eval += std::max<double>(0, spotShow.willingnessToPay - itinerary.returnPrice);
                }
            }
        }
    }
    return eval;
}

double SmartEstimator::estimateSpotMarket(
    const InputData& input,
    const InputLinks& links,
    const MarketData& market) const {
    double eval = 0;
    for (const auto & e : market.idEventToWpay) {
        for (const auto & p : e.second) {
            auto spotShows = p.second;
            std::sort(spotShows.begin(), spotShows.end(),
                [](const SpotShow& l, const SpotShow& r) {
                    return l.willingnessToPay < r.willingnessToPay;
            });

            const auto & itinerary = input.itineraries[p.first];

            double maxRevenue = 0;

            ui32 showSum = 0;
            for (ui32 i = 0; i < spotShows.size(); ++i) {
                if (spotShows[i].showFlag) {
                    showSum += 1;
                }
            }

            for (ui32 i = 0; i < spotShows.size(); ++i) {
                ui32 customerCount = spotShows.size() - i;
                ui32 noShowCount = customerCount = showSum;
                auto revenueNow = (spotShows[i].willingnessToPay - itinerary.cost) * showSum;
                revenueNow += (spotShows[i].willingnessToPay - itinerary.returnPrice) * noShowCount;
                if (maxRevenue < revenueNow) {
                    maxRevenue = revenueNow;
                }
                if (spotShows[i].showFlag) {
                    showSum -= 1;
                }
            }
            eval += maxRevenue;
        }

    }
    eval = std::min<double>(eval,
            BaseEstimator::estimateSpotMarket(input, links, market));
    return eval;
}

}   // namespace sea




