/**
 * @file common.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares commonly used methods and enumerations.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "types.h"
#include "input/input_data.h"

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <unordered_map>

namespace sea {

/// @brief Enumerates Convergence Criterion.
enum class ConvergenceCriterionType {
    VARIABLE_CHANGE,   ///< By small change in variables.
    OBJECTIVE_CHANGE,  ///< By small change in objective.
    BOTH               ///< Either small change in variables or in objective is enough.
};

/// @brief Enumerates Allotment Strategy types.
enum class AllotmentStrategyType {
    BENDERS,            ///< Benders Decomposition Heuristic.
    DET_CUT_PLANE,      ///< Deterministic Cutting Plane.
    ZERO_IPOPT,         ///< No allotments, preprocess the rest with Ipopt fluid approximation.
    ZERO_NULL,          ///< No allotments and no spot market.
    IPOPT               ///< Ipopt Fluid Approximation.
};

/// @brief Enumerates Spot Strategy types.
enum class SpotStrategyType {
    IPOPT,   ///< Ipopt fluid approximation.
    HYBRID,  ///< Hybrid approach. Use duals from Ipopt in LR mechanism.
    LR       ///< Lagrangial Relaxation.
};

/// @brief Enumerates backend type.
enum class BackendType {
    BENDERS,        ///< Benders decomposition backend.
    IPOPT,          ///< Ipopt Fluid approximation backend.
    DET_CUT_PLANE,  ///< Deterministic Cutting Plane backend.
    LR              ///< Lagrangian Relaxation backend.
};

/**
 * @brief Retrieves RAM memory usage by the executable.
 * 
 * @return Amount of RAM used (in bytes).
 */
std::size_t getMemUsage();
/**
 * @brief Writes the current time and amount of RAM used (in MB).
 * 
 * @param[out] Stream to write the information into.
 */
void printTimeMemNow(std::ostream& ostream);

/**
 * @brief Creates unique string filename.
 * 
 * @return The created filename.
 */
std::string makeUniqueFileName();

/**
 * @brief Prints algorithm execution story.
 * 
 * @param story Story object, maps measurement name into a vector of measurements.
 * @param dirNamePrefix Prefix path to the directory for creating file with export.
 * @param algoId Name of the algorithm.
 */
void printStory(const std::map<std::string, std::vector<double>>& story,
        std::string dirNamePrefix, std::string algoId);
/**
 * @brief Prints algorithm execution story into stream.
 * 
 * @param[in] story Story to print.
 * @param[out] out Stream for printing.
 */
void printStory(const std::map<std::string, std::vector<double>>& story, std::ofstream& out);

/**
 * @brief Counts the number of elements in the vector, and the number of elements which are
 * not equal to MAX_INDEX.
 * 
 * @param The vector to process.
 * @return Pair with two elements. First element is the number of elements not equal to MAX_INDEX,
 * the second element is the total number of elements in vector.
 */
std::pair<unsigned, unsigned> getUsefulIndexCount(const std::vector<unsigned>& target);
/**
 * @brief Counts the number of elements in the 2d-vector, and the number of elements which are
 * not equal to MAX_INDEX.
 * 
 * @param The 2d-vector to process.
 * @return Pair with two elements. First element is the number of elements not equal to MAX_INDEX,
 * the second element is the total number of elements in 2d-vector.
 */
std::pair<unsigned, unsigned> getUsefulIndexCount(
        const std::vector<std::vector<unsigned>>& target);

}   // namespace sea
