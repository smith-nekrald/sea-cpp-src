/**
 * @file io_functions.hpp
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Template method for InputLinks serialization.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "io_functions.h"
#include "../reader/input_reader.h"
#include "../reader/market_reader.h"

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>


namespace cereal {

/**
 * @brief Serialization method for InputLinks.
 * 
 * @tparam Archive The type of archive.
 * @param archive The archiving object.
 * @param links The InputLinks structure to serialize.
 */
template<class Archive>
void serialize(Archive& archive, sea::InputLinks& links) {
    archive(links.itinerariesFromArc,
        links.itinerariesWithArc,
        links.itinerariesToArc,
        links.demandTimesForItinerary,
        links.allotmentsWithItinerary,
        links.allotmentItineraryToEntry,
        links.allotmentItineraryToPlace,
        links.allotmentToGroups,
        links.firstArcToHireAfterArc,
        links.arrivalTime);
}

} // namespace cereal

