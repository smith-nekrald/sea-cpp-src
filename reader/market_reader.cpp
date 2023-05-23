// Implements defined methods for MarketReader.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "market_reader.h"
#include "common_reader.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <unordered_map>
#include <cassert>
#include <iterator>
#include <algorithm>

namespace sea {
namespace io {

std::ifstream& MarketReader::readEvents(std::ifstream& input, MarketData& data) const {
    std::string placeholder;   // id_event_to_wpay
    while ((placeholder.find("id_event_to_wpay") == std::string::npos)
            && std::getline(input, placeholder)) {
    }

    unsigned pricingEventCount;
    input >> pricingEventCount;

    for (unsigned idxEvent = 0; idxEvent < pricingEventCount; ++idxEvent) {
        unsigned relatedItineraryCount;
        input >> relatedItineraryCount;

        for (unsigned idxRelated = 0; idxRelated < relatedItineraryCount; ++idxRelated) {
            unsigned eventId;
            unsigned itineraryId;
            unsigned sampleLen;

            input >> eventId >> itineraryId >> sampleLen;

            if (data.idEventToWpay.find(eventId) == data.idEventToWpay.end()) {
                data.idEventToWpay.emplace(
                        eventId, std::unordered_map<unsigned, std::vector<SpotShow> >{});
            }

            double value;
            for (unsigned jdxEntry = 0; jdxEntry < sampleLen; ++jdxEntry) {
                input >> value;
                // Skipping further activity since the same information is repeated again.
            }
        }
    }
    return input;
}

std::ifstream& MarketReader::readAllotmentShows(std::ifstream& input, MarketData& data) const {
    std::string placeholder;   // Looking for allotment_show_count.
    while ((placeholder.find("allotment_show_count") == std::string::npos)
            && std::getline(input, placeholder)) {
    }

    unsigned allotmentCount;
    input >> allotmentCount;

    data.allotmentShowCount.resize(allotmentCount);
    for (unsigned idxAllotment = 0; idxAllotment < allotmentCount; ++idxAllotment) {
        unsigned allotmentId, count;
        input >> allotmentId >> count;
        for (unsigned jdxItem = 0; jdxItem < count; ++jdxItem) {
            unsigned itineraryId, showCount;
            input >> allotmentId >> itineraryId >> showCount;
            data.allotmentShowCount[allotmentId][itineraryId] = showCount;
        }
    }
    return input;
}

std::ifstream& MarketReader::readSpotShows(std::ifstream& input, MarketData& data) const {

    std::string placeholder;   // spot_show_tuples
    while ((placeholder.find("spot_show_tuples") == std::string::npos)
            && std::getline(input, placeholder)) {
    }

    unsigned itineraryCount;
    input >> itineraryCount;

    for (unsigned ind = 0; ind < itineraryCount; ++ind) {
        unsigned itineraryId, entriesLen;
        input >> itineraryId >> entriesLen;

        for (unsigned jdx = 0; jdx < entriesLen; ++jdx) {
            unsigned eventId, showFlag, index;
            double willingnessToPay;
            input >> itineraryId >> eventId >> index >> willingnessToPay >> showFlag;
            assert(showFlag == 0 || showFlag == 1);

            data.idEventToWpay[eventId][itineraryId].emplace_back(
                SpotShow{willingnessToPay, index, static_cast<bool>(showFlag)});
        }
    }

    for (auto& itineraries : data.idEventToWpay) {
        for (auto& item : itineraries.second) {
            item.second.resize(item.second.size());
            std::sort(std::begin(item.second), std::end(item.second),
                [](const SpotShow& lhs, const SpotShow& rhs){
                    return lhs.indexInSample < rhs.indexInSample;
            });
        }
    }

    return input;
}

const std::string MarketReader::header = "MarketData:";

void MarketReader::Do(const std::string& filePath, MarketData& data) const {
    std::ifstream inStream(filePath, std::ios_base::in);

    if (!inStream.good()) {
        throw std::runtime_error("Failed to open market file: " + filePath);
    }

    std::vector<std::string> tokens;
    makeTokens(inStream, tokens);
    assert(tokens[0] == header);

    readEvents(inStream, data);
    readAllotmentShows(inStream, data);
    readSpotShows(inStream, data);
}

}   // namespace io
}   // namespace sea


