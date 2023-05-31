/**
 * @file index_map.hpp
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Template methods for DCP Index Map.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "index_map.h"

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/base_class.hpp>

namespace cereal {

/**
 * @brief Template serialization for DcpIndexMap.
 * 
 * @tparam Archive Type of archive.
 * @param[out] archive The serialization archvie.
 * @param[out] indexMap The object to process.
 */
template<class Archive>
void serialize(Archive& archive, sea::backend::DcpIndexMap& indexMap) {
    archive(cereal::base_class<sea::backend::IpoptIndexMap>(&indexMap),
        indexMap.timeItineraryToZi,
        indexMap.demandZCount);
}

} // namespace cereal


