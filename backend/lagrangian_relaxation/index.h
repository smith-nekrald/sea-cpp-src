/**
 * @file index.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines LR Index and related API.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../../input/input_data.h"
#include "../../input/input_links.h"
#include "../dual_variables.h"

#include <vector>
#include <unordered_map>

namespace sea {
namespace backend {

using std::vector;
using std::unordered_map;

/**
 * @brief Entity to help with indexation in Lagrangian Relaxation.
 */
struct LagrangianRelaxationIndex {
    /// @brief Maps arc index to corresponding lambda index, if relevant.
    vector<unsigned> arcToLambdaIndex;
    /// @brief Number of lambda variables.
    unsigned lambdaCount = 0;
    /// @brief Number of mu varaibles.
    unsigned muCount = 0;
};

/**
 * @brief Initializes lagrangian relaxation index.
 * 
 * @param[in] links Input Links, data structure over Input Data.
 * @param[in] input Input Data, static information.
 * @param[out] index The index to fill.
 */
void initLagrangianRelaxationIndex(const InputLinks& links,
        const InputData& input,
        LagrangianRelaxationIndex* index);
/**
 * @brief Initializes dual variables.
 * 
 * @param[in] index LR Index for indexation.
 * @param[out] dualVariables Dual variables to initialize.
 */
void initDuals(const LagrangianRelaxationIndex& index, DualVariables* dualVariables);

} // namespace backend

/**
 * @brief Writes LR index to file.
 * 
 * @param[in] pathToFile Path to file.
 * @param[in] index  Index to write.
 */
void writeToFile(const std::string& pathToFile, const backend::LagrangianRelaxationIndex& index);
/**
 * @brief Reads LR index from file.
 * 
 * @param[in] pathToFile Path to file.
 * @param[out] index Index to read.
 */
void loadFromFile(const std::string& pathToFile, backend::LagrangianRelaxationIndex* index);

} // namespace sea
