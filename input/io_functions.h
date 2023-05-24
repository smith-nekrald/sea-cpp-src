/**
 * @file io_functions.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines loadFromFile and writeToFile methods for InputData, InputLinks and MarketData.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "input_data.h"
#include "market_data.h"
#include "input_links.h"

#include <string>

namespace sea {

/**
 * @brief Method to load InputData from file.
 * @param[in] pathToFile The path to file with InputData.
 * @param[out] data The place to load the InputData.
 */
void loadFromFile(const std::string& pathToFile, InputData* data);
/**
 * @brief Method to export InputData to file. Since InputData is already stored in some file
 * at input, this method does nothing. 
 * 
 * @param pathToFile Path to file.
 * @param data The InputData to process.
 */
void writeToFile(const std::string& pathToFile, const InputData& data);

/**
 * @brief Method to read MarketData from file. 
 * 
 * @param[in] pathToFile  The path to file with MarketData.
 * @param[out] data The place to load the MarketData.
 */
void loadFromFile(const std::string& pathToFile, MarketData* data);
/**
 * @brief Method to export MarketData to file. Since MarketData is already stored in some file,
 * this method does nothing.
 * 
 * @param pathToFile Path to file.
 * @param data The MarketData to process.
 */
void writeToFile(const std::string& pathToFile, const MarketData& data);

/**
 * @brief Method to load InputLinks from file.
 * 
 * @param[in] pathToFile Path to file with InputLinks.
 * @param[out] links The place to load the InputLinks.
 */
void loadFromFile(const std::string& pathToFile, InputLinks* links);
/**
 * @brief Method to export InputLinks to file.
 * 
 * @param pathToFile Path to file to write InputLinks to.
 * @param links The structure to store.
 */
void writeToFile(const std::string& pathToFile, const InputLinks& links);

} // namespace sea
