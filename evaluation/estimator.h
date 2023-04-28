#pragma once

#include "../input/input_data.h"
#include "../input/market_data.h"
#include "../input/input_links.h"

#include <cassert>
#include <algorithm>
#include <ostream>

namespace sea {

struct RevenueEstimate {
    double allotment = 0;
    double spotMarket = 0;

    friend std::ostream& operator<<(
            std::ostream& out, const RevenueEstimate& estimate);
};

class BaseEstimator {
public:
    RevenueEstimate calcUpperBound(
            const InputData& i,
            const InputLinks& l,
            const MarketData& m) const {
        RevenueEstimate eval;
        assert(eval.allotment == 0);
        assert(eval.spotMarket == 0);
        eval.allotment = estimateAllotments(i, l, m);
        eval.spotMarket = estimateSpotMarket(i, l, m);
        return eval;
    }

    virtual ~BaseEstimator() {}

protected:
    virtual double estimateAllotments(
        const InputData&, const InputLinks&, const MarketData&) const;
    virtual double estimateSpotMarket(
        const InputData&, const InputLinks&, const MarketData&) const;
};

class SmartEstimator : public BaseEstimator {
private:
    virtual double estimateSpotMarket(
        const InputData&, const InputLinks&, const MarketData&) const override;
};

}   // namespace sea
