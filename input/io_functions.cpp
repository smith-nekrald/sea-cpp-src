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

template<class Archive>
void serialize(Archive& ar, sea::InputLinks& links)
{
    ar(links.itinerariesFromArc,
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


namespace sea {

void loadFromFile(const std::string& pathToFile, InputData* data) {
    io::InputReader reader;
    reader.Do(pathToFile, *data);
}

void writeToFile(const std::string& /*pathToFile*/, const InputData& /*data*/) {
    // Do nothing, since the file already exists.
}

void loadFromFile(const std::string& pathToFile, MarketData* data) {
    io::MarketReader reader;
    reader.Do(pathToFile, *data);
}

void writeToFile(const std::string& /*pathToFile*/, const MarketData& /*data*/) {
    // Do nothing, since the file already exists.
}

void loadFromFile(const std::string& pathToFile, InputLinks* links) {
    std::ifstream reader(pathToFile, std::ios::binary);
    ::cereal::BinaryInputArchive ia(reader);
    ia >> *links;
}

void writeToFile(const std::string& pathToFile, const InputLinks& links) {
    std::ofstream writer(pathToFile, std::ios::binary);
    ::cereal::BinaryOutputArchive oa(writer);
    oa << links;
}

} // namespace sea
