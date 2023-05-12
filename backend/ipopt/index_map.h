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

/**
 * @brief Index map is a structure to convert the original input indexation into the internal 
 * indexation that suits optimization.
 */
struct IpoptIndexMap {

// Indices of independent variables.

    /// @brief Converts relative time and itinerary index into demand variable (d_t) index.
    std::vector<std::vector<unsigned>> timeItineraryToDemandIndex;   
    /// @brief Converts arc index into the variable index for the amount of 
    /// hired y_a^H on cut-off from arc a.
    std::vector<unsigned> arcToHired; 
    /// @brief Converts [itinerary index] into the Q_r variable index (non-empty spot TEU amount).
    std::vector<unsigned> idItineraryToQIndex; 
    /// @brief Converts [itinearry index] into the Z_r variable index (emtpy TEU amount).
    std::vector<unsigned> idItineraryToZIndex; 
    /// @brief Converts [id_allotment] and [id_itinerary] to the Q_i^r variable 
    /// index (non-empty allotment TEU amount).
    std::vector<std::vector<unsigned>> allotmentItineraryToQIndex; 
    /// @brief Converts [id_allotment] to the u_i variable index (allotment accept decision).
    std::vector<unsigned> allotmentToUIndex; 
    /// @brief Converts [time] to s_t, decision at time t for offhiring at the arrival port. 
    std::vector<unsigned> timeToSIndex; 

    /// @brief Total variable count. Helps for creating corresponding vectors.
    unsigned variableCount;

// Indices of constraints. (Numeration starts from 1,
// since 0 is for objective function).

    /// @brief  Maps [arc index] to the corresponding capacity constraint index, 
    /// i.e. load_on_arc <= W_a.
    std::vector<unsigned> arcCapacityConstraints; 
    /// @brief Maps [arc index] to the constraint index for TEU balance on cut-off.
    std::vector<unsigned> portPositiveCutoffArcConstraints; 
    /// @brief Maps [arc index] to the constraint index for TEU balance on arrival in port.
    std::vector<unsigned> portPositiveArrivalArcConstraints; 
    /// @brief Maps [itinerary index] to the index of the constraint  Q <= N for spot market.
    std::vector<unsigned> spotQNConstraints; 
    /// @brief Maps [port index] to the final TEU balance constraint in port index.
    std::vector<unsigned> finalContainerConstraints; 
    /// @brief Maps [group id] to the group constraint index.
    std::vector<unsigned> groupConstraints; 
    /// @brief Maps [allotment id][itinerary id] to indices for constraitns $Q_i^r \leq u_i EN_i^r$.
    std::vector<std::vector<unsigned>> allotmentItineraryQConstraints; 

    /// @brief Total amount of constraints. Helps for creating corresponding vectors.
    unsigned constraintCount;

    /// @brief Maps variable index to a human-readable description.
    std::map<unsigned, std::string> variableToDescription; 
    /// @brief Maps constraint index to a human-readable description.
    std::map<unsigned, std::string> constraintToDescription;


};

void initIndexMapWithMaxIndex(const InputData& input, IpoptIndexMap* indexMap);
void createIndexMap(const InputData& input,
        IpoptIndexMap* indexMap,
        bool initDescriptions=false);
void addIpoptIndexMap(const InputData& input,
        IpoptIndexMap* indexMap,
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
