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
            const InputData& input, const InputLinks& links, const MarketData& market);

    virtual ~BaseEstimator() {}

protected:
    virtual double estimateAllotments(
        const InputData& input, const InputLinks& links, const MarketData& market);
    virtual double estimateSpotMarket(
        const InputData& input, const InputLinks& links, const MarketData& market);
    void estimateEachAllotment(
        const InputData& input, const InputLinks& links, const MarketData& market);

protected:
    std::vector<double> allotmentProfitEstimation;
};

class SmartEstimator : public BaseEstimator {
private:
    virtual double estimateSpotMarket(
        const InputData& input, const InputLinks& links, const MarketData& market) override;
    virtual double estimateAllotments(
        const InputData& input, const InputLinks& links, const MarketData& market) override;
};

}   // namespace sea
