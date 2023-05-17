/**
 * @file index_map.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements IpoptIndexMap, a structure for indexing optimization problem and converting
 * between solution of the optimization problem and backend output. Also implements some logging
 * and IO-related API.
 * @copyright (c) Smith School of Business, 2023
 */
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

/**
 * @brief Creates a pre-initialized Ipopt index map. Reshapes corresponding vectors and 
 * sets default values.
 * 
 * @param[in] input The InputData entity to derive the required parameters.
 * @param[out] indexMap  The IndexMap object to initialize.
 * @param[in] initDescriptions If true, initialize human-readable description. 
 */
void createIndexMap(const InputData& input, IpoptIndexMap* indexMap, bool initDescriptions=false);

/**
 * @brief Reshapes Ipopt index map vectors to correct shapes and initializes with MAX UNSIGNED.
 * 
 * @param[in] input The InputData entity to derive the required parameters.
 * @param[out] indexMap The IpoptIndexMap to process.
 */
void initIndexMapWithMaxIndex(const InputData& input, IpoptIndexMap* indexMap);

/**
 * @brief Forms indices in the IpoptIndexMap and supplies Ipopt Index Map with human-readable 
 * descriptions.
 * 
 * @param[in] input The InputData entity to derive the required parameters.
 * @param[out] indexMap The IpoptIndexMap entity to fill.
 * @param[in] initDescriptions If true, initializes human-readable descriptions.
 */
void addIpoptIndexMap(const InputData& input, IpoptIndexMap* indexMap, bool initDescriptions=false);

/**
 * @brief Template method to export indexMap into a human-readable form.
 * 
 * @tparam Writer  The type of the out object. Potentially ifstream, ofstream or logger.
 * @param out The stream object to write. 
 * @param indexMap  The Ipopt Index Map entity to export.
 */
template<typename Writer> 
void printIndexMapStatsGeneral(Writer&  out, const IpoptIndexMap& indexMap);

/**
 * @brief Prints index map into output stream.
 * 
 * @param out  The output stream for writing.
 * @param indexMap  The Ipopt Index Map to export.
 */
void printIndexMapStats(std::ostream& out, const IpoptIndexMap& indexMap);

/**
 * @brief Prints Ipopt index map into logger.
 * 
 * @param logger The logger to stream into.
 * @param indexMap  The entity to stream into the logger.
 */
void printIndexMapStats(log4cpp::CategoryStream& logger, const IpoptIndexMap& indexMap);


} // namespace backend

/**
 * @brief  Writes IpoptIndexMap to a file.
 * 
 * @param filePath The path of the export file.
 * @param indexMap  The entity to export.
 */
void writeToFile(const std::string& filePath, const backend::IpoptIndexMap& indexMap);

/**
 * @brief  Loads IpoptIndexMap from file.
 * 
 * @param[in] filePath The path of the file.
 * @param[out] indexMap  The entity to load.
 */
void loadFromFile(const std::string& filePath, backend::IpoptIndexMap* indexMap);

} // namespace sea

namespace cereal {

/**
 * @brief  Method to allow exporting InpoptIndexMap with cereal.
 * 
 * @tparam Archive Type of cereal output archive.
 * @param ar The archive object. 
 * @param indexMap The entity to serialize.
 */
template<class Archive>
void serialize(Archive& ar, sea::backend::IpoptIndexMap& indexMap) {
    // Specify all relevant serialization fields.
    ar( indexMap.timeItineraryToDemandIndex,
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
