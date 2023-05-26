/**
 * @file market_data.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines MarketData, which represents a single trajectory of environment realization.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <unordered_map>
#include <memory>
#include <vector>

namespace sea {

/**
 * @brief Represents each client in Spot Market show sample.
 * Essentially, describes willingness-to-pay and whether the client shows.
 */
struct SpotShow {
    /// @brief Willigness to pay is the maximal price the client is ready to pay for service.
    double willingnessToPay;
    /// @brief Index of the client in sample.
    unsigned indexInSample;
    /// @brief If true, the client shows. If false, the client no-shows.
    bool showFlag;
};

/**
 * @brief MarketData represents a single realization of the scenario assumed in InputData.
 * A trajectory of clients with their demands and behaviors.
 */
struct MarketData {
    /// @brief Maps pricing event index and itinerary to SpotShow entry
    /// with willingness to pay and show information. Indexation is as follows:
    /// [id_event(pricing)][id_itinerary] -> array of buyers with their willingness to pay, etc.
    std::unordered_map<unsigned, std::unordered_map<unsigned, std::vector<SpotShow>>> idEventToWpay;
    /// @brief Maps [id_allotment][id_itinerary] -> shown amount for that itinerary.
    std::vector<std::unordered_map<unsigned, unsigned> > allotmentShowCount;
};

/// @brief Simple notation for a shared pointer to MarketData.
using MarketDataPtr=std::shared_ptr<MarketData>;
/// @brief Simple notation for a shared pointer to const MarketData.
using MarketDataConstPtr=std::shared_ptr<const MarketData>;

}   // namespace sea

