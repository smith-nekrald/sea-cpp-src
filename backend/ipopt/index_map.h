#pragma once

#include "../../types.h"
#include "../../input/input_data.h"
#include "../../logging/logging.h"

#include <vector>
#include <map>

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/base_class.hpp>

namespace sea {
namespace backend {

struct IpoptIndexMap {

// Indices of independent variables.
    std::vector<std::vector<unsigned>> timeItineraryToDemandIndex;   // d_t
    std::vector<unsigned> arcToHired; // y_a^H, on cut-off from arc a

    std::vector<unsigned> idItineraryToQIndex; // Q_r
    std::vector<unsigned> idItineraryToZIndex; // Z_r
    std::vector<std::vector<unsigned>> allotmentItineraryToQIndex; // Q_i^r

    std::vector<unsigned> allotmentToUIndex; // u_i
    std::vector<unsigned> timeToSIndex; // s_t, decision at time t for port of arrival

    unsigned variableCount;

// Indices of constraints. (Numeration starts from 1,
// since 0 is for objective function).

    std::vector<unsigned> arcCapacityConstraints; // indexed by arc, load_on_arc <= W_a
    std::vector<unsigned> portPositiveCutoffArcConstraints; // indexed by arc
    std::vector<unsigned> portPositiveArrivalArcConstraints; // indexed by arc
    std::vector<unsigned> spotQNConstraints; // indexed by itinerary
    std::vector<unsigned> finalContainerConstraints; // indexed by port
    std::vector<unsigned> groupConstraints; // indexed by group id
    unsigned constraintCount;

    std::map<unsigned, std::string> variableToDescription, constraintToDescription;

    // This is used only in enhanced version or dcp!
    std::vector<std::vector<unsigned>> allotmentItineraryQConstraints; // Q_i^r \le u_i EN_i^r
};

void initIndexMapWithMaxIndex(const InputData& input, IpoptIndexMap* indexMap);
void createIndexMap(const InputData& input,
        IpoptIndexMap* indexMap,
        bool useEnhancement=false,
        bool initDescriptions=false);
void addIpoptIndexMap(const InputData& input,
        IpoptIndexMap* indexMap,
        bool useEnhancement=false,
        bool initDescriptions=false);
void printIndexMapStats(std::ostream& out, const IpoptIndexMap& indexMap);
void printIndexMapStats(log4cpp::CategoryStream& logger, const IpoptIndexMap& indexMap);


} // namespace backend

void writeToFile(const std::string& filePath, const backend::IpoptIndexMap& indexMap);
void loadFromFile(const std::string& filePath, backend::IpoptIndexMap* indexMap);

} // namespace sea

namespace cereal {

template<class Archive>
void serialize(Archive& ar, sea::backend::IpoptIndexMap& indexMap)
{
    ar(indexMap.timeItineraryToDemandIndex,

        indexMap.arcToHired,

        indexMap.idItineraryToQIndex,
        indexMap.idItineraryToZIndex,
        indexMap.allotmentItineraryToQIndex,

        indexMap.allotmentToUIndex,
        indexMap.timeToSIndex,

        indexMap.variableCount,

        indexMap.arcCapacityConstraints,
        indexMap.portPositiveCutoffArcConstraints,
        indexMap.portPositiveArrivalArcConstraints,
        indexMap.spotQNConstraints,
        indexMap.finalContainerConstraints,
        indexMap.groupConstraints,
        indexMap.constraintCount,

        indexMap.variableToDescription,
        indexMap.constraintToDescription,

        indexMap.allotmentItineraryQConstraints);
}

} // namespace cereal
