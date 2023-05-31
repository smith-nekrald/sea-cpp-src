/**
 * @file index_map.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares DCP Index Map - manager for indices used in Deterministic Cutting Plane method.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../ipopt/index_map.h"
#include "../../input/input_links.h"
#include "../../input/input_data.h"


namespace sea {
namespace backend {

/**
 * @brief Index map. Uses the same indexation model like IpoptIndexMap, but in addition
 * adds entries specific for Det-Cut-Plane allotment.
 */
struct DcpIndexMap: IpoptIndexMap {
    /// @brief Maps time-itinerary to Zi index.
    std::vector<std::vector<unsigned>> timeItineraryToZi;
    /// @brief First <demandZCount> variables from variableCount are z_i
    unsigned demandZCount = 0;
};

/**
 * @brief Initialization for DcpIndexMap.
 * 
 * @param[in] input InputData to use for initialization.
 * @param[out] indexMap IndexMap to initialize.
 */
void dcpCreateIndexMap(const InputData& input, DcpIndexMap& indexMap);
} // namespace backend

/**
 * @brief Exports DCP Index Map to file.
 * 
 * @param filePath Export path.
 * @param indexMap Entity to export.
 */
void writeToFile(const std::string& filePath, const backend::DcpIndexMap& indexMap);
/**
 * @brief Reads DCP Index Map from file.
 * 
 * @param[out] filePath Path to read from.
 * @param[in] indexMap Entity to import.
 */
void loadFromFile(const std::string& filePath, backend::DcpIndexMap* indexMap);

} // namespace sea
