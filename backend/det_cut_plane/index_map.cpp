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
void serialize(Archive& ar, sea::backend::DcpIndexMap& indexMap)
{
    ar(cereal::base_class<sea::backend::IpoptIndexMap>(&indexMap),
        indexMap.timeItineraryToZi,
        indexMap.demandZCount);
}

} // namespace cereal


namespace sea {
namespace backend {

const ui32 MAX_INDEX = std::numeric_limits<ui32>::max();

void dcpCreateIndexMap(const InputData& input, DcpIndexMap& indexMap) {
    initIndexMapWithMaxIndex(input, &indexMap);

    indexMap.timeItineraryToZi.resize(input.events.size());
    indexMap.timeItineraryToZi.shrink_to_fit();
    for (auto& subVector : indexMap.timeItineraryToZi) {
        subVector.assign(input.itineraries.size(), MAX_INDEX);
        subVector.shrink_to_fit();
    }

    for (const auto& event : input.events) {
        ui32 relativeTime = event.relativeTime;
        if (event.type == InputData::Event::Type::pricing) {
            for (ui32 idItinerary : event.relatedItineraryIds) {
                indexMap.timeItineraryToZi[relativeTime][idItinerary] = indexMap.demandZCount++;
                indexMap.variableCount++; // First demandZCount variables are z_i
            }
        }
    }

    addIpoptIndexMap(input, &indexMap, true);
}

} // namespace backend

void writeToFile(
        const std::string& filePath,
        const backend::DcpIndexMap& indexMap) {
    std::ofstream writer(filePath, std::ios::binary);
    ::cereal::BinaryOutputArchive oa(writer);
    oa << indexMap;
}

void loadFromFile(
        const std::string& filePath,
        backend::DcpIndexMap* indexMap) {
    std::ifstream reader(filePath, std::ios::binary);
    ::cereal::BinaryInputArchive ia(reader);
    ia >> *indexMap;
}

} // namespace sea