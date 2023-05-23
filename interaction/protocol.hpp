/**
 * @file protocol.hpp
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements template IO for Decision and Action.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "protocol.h"

#include <iostream>
#include <algorithm>

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/base_class.hpp>

namespace cereal {

/**
 * @brief Serialization for Decision.
 * 
 * @tparam The Exporter type.
 * @param ar The Exporter object.
 * @param decision The decision to export.
 */
template<class Archive>
void serialize(Archive& ar, sea::Decision& decision) {
    ar(decision.time, 
        decision.offHiredInPortS,
        decision.prices,
        decision.emptyContainersZ,
        decision.nonEmptyContainersQ,
        decision.allotmentContainersQ,
        decision.hiredY,
        decision.allotmentAccepted);
}

/**
 * @brief Serialization for Action.
 * 
 * @tparam Archive The exporter type.
 * @param ar The exporter object.
 * @param action The action to export.
 */
template<class Archive>
void serialize(Archive& ar, sea::Action& action) {
    ar(action.time,
        action.spotMarketDemandN,
        action.bookingsB,
        action.allotmentDemandN);
}

} // namespace cereal


namespace sea {

using std::endl;
using std::size_t;

/**
 * @brief Template method to write Decision into stream.
 *
 * @tparam Type The type of stream.
 * @param[out] out The stream object.
 * @param[in] decision The decision to export.
 */
template<typename Type>
void writeToStream(Type& out, const Decision& decision) {
    out << "================" << endl;
    out << "Decision:" << endl;
    out << "timeNow " << decision.time << endl;
    out << "off-hired_in_ports_(by_ports):" << endl;
    out << decision.offHiredInPortS.size() << endl;

    for (size_t idxTime = 0; idxTime < decision.offHiredInPortS.size(); ++idxTime) {
        out << "time " << idxTime << " size " << decision.offHiredInPortS[idxTime].size() << endl;
        for (auto count : decision.offHiredInPortS[idxTime]) {
            out << count << " ";
        }
        out << endl;
    }
    out << "prices_[time][print_pairs:_<itinerary,price>]:" << endl;
    out << "time_size " << decision.prices.size() << endl;
    for (size_t idxTime = 0; idxTime < decision.prices.size(); ++idxTime) {
        out << "time " << idxTime << endl;
        out << "count " << decision.prices[idxTime].size() << endl;
        for (const auto& pairSpot : decision.prices[idxTime]) {
            out << pairSpot.first << " " << pairSpot.second << " " << endl;
        }
    }
    out << "empty_containers_(Z)_(by_itineraries):" << endl;
    out << "size " << decision.emptyContainersZ.size() << endl;
    for (auto zEmpty : decision.emptyContainersZ) {
        out << zEmpty << " ";
    }
    out << endl;

    out << "size " << decision.nonEmptyContainersQ.size() << endl;
    out << "non-empty_containers_(Q)_(by itineraries):" << endl;
    for (auto qNonEmpty : decision.nonEmptyContainersQ) {
        out << qNonEmpty << " ";
    }
    out << endl;

    out << "allotment_non-empty_containers_(Q)[contractId][<itinerary,q>:" << endl;
    out << "size " << decision.allotmentContainersQ.size() << endl;
    for (size_t contractId = 0; contractId < decision.allotmentContainersQ.size(); ++contractId) {
        out << "contract-id " << contractId << endl;
        out << "size " << decision.allotmentContainersQ[contractId].size() << endl;
        for (const auto& pairLong : decision.allotmentContainersQ[contractId]) {
            out << pairLong.first << " " << pairLong.second << " " << endl;
        }
    }
    out << "hired_containers_(Y)_(by_arcs):" << endl;
    for (auto hired : decision.hiredY) {
        out << hired << " ";
    }
    out << endl;

    out << "accepted_allotments:" << endl;
    out << "size " << decision.allotmentAccepted.size() << endl;
    for (size_t idx  = 0; idx < decision.allotmentAccepted.size(); ++idx) {
        if (decision.allotmentAccepted[idx]) {
            out << idx << " ";
        }
    }
    out << endl;
    out << "================" << endl;

}

/**
 * @brief Template method to write Action into stream.
 *
 * @tparam Type The stream type.
 * @param[out] writer The stream object.
 * @param[in] filledAction The action to write.
 */
template<typename Type>
void writeToStream(Type& writer, const Action& filledAction) {
    writer << "================" << endl;
    writer << "Action: " << endl;
    writer << "Time = " << filledAction.time << endl;
    writer << "spotMarketDemand_(N)_(by_itineraries):" << endl;
    writer << filledAction.spotMarketDemandN.size() << endl;
    for (const auto& value : filledAction.spotMarketDemandN) {
        writer << value  << " ";
    }
    writer << endl;
    writer << "bookings_(B)_(by_time_and_itineraries): " << endl;
    writer << filledAction.bookingsB.size() << endl;
    for (size_t time = 0; time < filledAction.bookingsB.size(); ++time) {
        auto& bookings = filledAction.bookingsB[time];
        writer << "time: " <<  time << "itineraries_count: "
            <<  filledAction.bookingsB[time].size() << endl;
        writer << "bookings: (indexed by itineraries) " << endl;
        for (const auto& value : bookings) {
            writer << value << " ";
        }
        writer << endl;
    }
    writer << "allotment_demand_(N)_(by_allotments):" << endl;
    writer << filledAction.allotmentDemandN.size() << endl;
    for (size_t idx = 0; idx < filledAction.allotmentDemandN.size(); ++idx) {
        writer << "contract_id " << idx << endl;
        for (const auto& pairLong : filledAction.allotmentDemandN[idx]) {
            writer << pairLong.first << " " << pairLong.second << " " << endl;
        }
    }
    writer << "================" << endl;
}


} // namespace sea
