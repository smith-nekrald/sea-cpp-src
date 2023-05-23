#pragma once

#include "../types.h"

#include <unordered_map>
#include <memory>
#include <vector>

namespace sea {

struct SpotShow {
    double willingnessToPay;
    unsigned indexInSample;
    bool showFlag;
};

struct MarketData {
    // [id_event(pricing)][id_itinerary] -> array of buyers with willingness to pay
    std::unordered_map<unsigned, std::unordered_map<unsigned, std::vector<SpotShow>>> idEventToWpay;

    // [id_allotment][id_itinerary] -> shown amount for itinerary
    std::vector<std::unordered_map<unsigned, unsigned> > allotmentShowCount;
};

using MarketDataPtr=std::shared_ptr<MarketData>;
using MarketDataConstPtr=std::shared_ptr<const MarketData>;

}   // namespace sea

