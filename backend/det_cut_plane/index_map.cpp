// Implementation for API related to IndexMap.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "index_map.h"
#include "index_map.hpp"


namespace sea {
namespace backend {

void dcpCreateIndexMap(const InputData& input, DcpIndexMap& indexMap) {
    initIndexMapWithMaxIndex(input, &indexMap);
    indexMap.timeItineraryToZi.resize(input.events.size());
    indexMap.timeItineraryToZi.shrink_to_fit();
    for (auto& subVector : indexMap.timeItineraryToZi) {
        const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();
        subVector.assign(input.itineraries.size(), MAX_INDEX);
        subVector.shrink_to_fit();
    }
    for (const auto& event : input.events) {
        unsigned relativeTime = event.relativeTime;
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned idItinerary : event.relatedItineraryIds) {
                indexMap.timeItineraryToZi[relativeTime][idItinerary] = indexMap.demandZCount++;
                indexMap.variableCount++; // First demandZCount variables are z_i
            }
        }
    }
    addIpoptIndexMap(input, &indexMap, true);
}

} // namespace backend

void writeToFile(const std::string& filePath, const backend::DcpIndexMap& indexMap) {
    std::ofstream writer(filePath, std::ios::binary);
    ::cereal::BinaryOutputArchive outputArchive(writer);
    outputArchive << indexMap;
}

void loadFromFile(const std::string& filePath, backend::DcpIndexMap* indexMap) {
    std::ifstream reader(filePath, std::ios::binary);
    ::cereal::BinaryInputArchive inputArchive(reader);
    inputArchive >> *indexMap;
}

} // namespace sea
