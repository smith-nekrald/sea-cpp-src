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
    std::vector<std::vector<ui32>> timeItineraryToDemandIndex;   // d_t
    std::vector<ui32> arcToHired; // y_a^H, on cut-off from arc a

    std::vector<ui32> idItineraryToQIndex; // Q_r
    std::vector<ui32> idItineraryToZIndex; // Z_r
    std::vector<std::vector<ui32>> allotmentItineraryToQIndex; // Q_i^r

    std::vector<ui32> allotmentToUIndex; // u_i
    std::vector<ui32> timeToSIndex; // s_t, decision at time t for port of arrival

    ui32 variableCount;

// Indices of constraints. (Numeration starts from 1,
// since 0 is for objective function).

    std::vector<ui32> arcCapacityConstraints; // indexed by arc, load_on_arc <= W_a
    std::vector<ui32> portPositiveCutoffArcConstraints; // indexed by arc
    std::vector<ui32> portPositiveArrivalArcConstraints; // indexed by arc
    std::vector<ui32> spotQNConstraints; // indexed by itinerary
    std::vector<ui32> finalContainerConstraints; // indexed by port
    std::vector<ui32> groupConstraints; // indexed by group id
    ui32 constraintCount;

    std::map<ui32, std::string> variableToDescription, constraintToDescription;

    // This is used only in enhanced version or dcp!
    std::vector<std::vector<ui32>> allotmentItineraryQConstraints; // Q_i^r \le u_i EN_i^r
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
