#include "market_reader.h"

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

namespace {

template <typename T = std::string>
inline void makeTokens(const std::string& s, std::vector<T>& tokens) {

    std::istringstream iss(s);

    std::vector<T> tmp{std::istream_iterator<T>{iss}, std::istream_iterator<T>{}};

    tokens.swap(tmp);
}

template <typename T = std::string>
inline std::ifstream& makeTokens(std::ifstream& stream, std::vector<T>& tokens) {

    std::string to_parse;
    while (to_parse.empty())
        std::getline(stream, to_parse);

    makeTokens<T>(to_parse, tokens);

    return stream;
}

}   // namespace

std::ifstream& MarketReader::readEvents(std::ifstream& ifs, MarketData& data) const {

    std::string fake;   // id_event_to_wpay
    while((fake.find("id_event_to_wpay") == std::string::npos) && std::getline(ifs, fake));

    ui32 pricingEventCount;
    ifs >> pricingEventCount;

    for (ui32 i = 0; i < pricingEventCount; ++i) {
        ui32 relatedItineraryCount;
        ifs >> relatedItineraryCount;

        for (ui32 i = 0; i < relatedItineraryCount; ++i) {
            ui32 eventId;
            ui32 itineraryId;
            ui32 sampleLen;

            ifs >> eventId >> itineraryId >> sampleLen;

            if (data.idEventToWpay.find(eventId) == data.idEventToWpay.end()) {
                data.idEventToWpay.emplace(eventId, std::unordered_map<ui32, std::vector<SpotShow> >{});
            }

            double value;
            for (ui32 j = 0; j < sampleLen; ++j) {
                ifs >> value;
            }
        }
    }

    return ifs;
}

std::ifstream& MarketReader::readAllotmentShows(std::ifstream& ifs, MarketData& data) const {

    std::string fake;   // allotment_show_count
    while ((fake.find("allotment_show_count") == std::string::npos) && std::getline(ifs, fake));

    ui32 allotmentCount;
    ifs >> allotmentCount;

    data.allotmentShowCount.resize(allotmentCount);

    for (ui32 i = 0; i < allotmentCount; ++i) {
        ui32 allotmentId, count;
        ifs >> allotmentId >> count;

        for (ui32 j = 0; j < count; ++j) {
            ui32 itineraryId, showCount;
            ifs >> allotmentId >> itineraryId >> showCount;

            data.allotmentShowCount[allotmentId][itineraryId] = showCount;
        }
    }

    return ifs;
}

std::ifstream& MarketReader::readSpotShows(std::ifstream& ifs, MarketData& data) const {

    std::string fake;   // spot_show_tuples
    while ((fake.find("spot_show_tuples") == std::string::npos) && std::getline(ifs, fake));

    ui32 itineraryCount;
    ifs >> itineraryCount;

    for (ui32 i = 0; i < itineraryCount; ++i) {
        ui32 itineraryId, entriesLen;
        ifs >> itineraryId >> entriesLen;

        for (ui32 j = 0; j < entriesLen; ++j) {
            ui32 eventId, showFlag, index;
            double willingnessToPay;
            ifs >> itineraryId >> eventId >> index >> willingnessToPay >> showFlag;
            assert(showFlag == 0 || showFlag == 1);

            data.idEventToWpay[eventId][itineraryId].emplace_back(
                SpotShow{willingnessToPay, index, static_cast<bool>(showFlag)});
        }
    }

    for (auto & itineraries : data.idEventToWpay) {
        for (auto & i : itineraries.second) {
            i.second.resize(i.second.size());
            std::sort(std::begin(i.second), std::end(i.second),
                [](const SpotShow& l, const SpotShow& r){
                    return l.indexInSample < r.indexInSample;
            });
        }
    }

    return ifs;
}

const std::string MarketReader::header = "MarketData:";

void MarketReader::Do(const std::string& filepath, MarketData& data) const {

    std::ifstream ifs(filepath, std::ios_base::in);

    if (!ifs.good())
        throw std::runtime_error("failed to open market file: " + filepath);

    std::vector<std::string> tokens;
    makeTokens(ifs, tokens);

    assert(tokens[0] == header);

    readEvents(ifs, data);

    readAllotmentShows(ifs, data);

    readSpotShows(ifs, data);
}

}   // namespace io
}   // namespace sea


