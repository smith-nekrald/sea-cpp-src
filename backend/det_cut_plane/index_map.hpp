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

template<class Archive>
void serialize(Archive& archive, sea::backend::DcpIndexMap& indexMap) {
    archive(cereal::base_class<sea::backend::IpoptIndexMap>(&indexMap),
        indexMap.timeItineraryToZi,
        indexMap.demandZCount);
}

} // namespace cereal


