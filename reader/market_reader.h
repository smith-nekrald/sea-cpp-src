/**
 * @file market_reader.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines entity for reading MarketData, called MarketReader.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <string>
#include <fstream>

#include "../input/market_data.h"
#include "ireader.h"

namespace sea {
namespace io {

/**
 * @brief Entity to read market data.
 */
class MarketReader : public IReader<MarketData> {
public:
    /**
     * @brief Entry point. Method to read market data.
     * 
     * @param[in] filePath The path to file for reading from.
     * @param[out] data The data to read.
     */
    void Do(const std::string& filePath, MarketData& data) const override;
    /**
     * @brief Virtual destructor to ensure correct polymorphism.
     */
    virtual ~MarketReader() {};

private:
    /// @brief Header with which the Market Data starts.
    static const std::string header;    // MarketData:

    /**
     * @brief Reads events from Market Data from stream.
     * 
     * @param[out] inStream The stream to read the information from.
     * @param[out] data The MarketData to write the read information into.
     * @return The updated stream after reading.
     */
    std::ifstream& readEvents(std::ifstream& inStream, MarketData& data) const;
    /**
     * @brief Reads allotment show information from stream.
     * 
     * @param[out] inStream The stream to read the information from.
     * @param[out] data The MarketData to write the read information into.
     * @return 
     */
    std::ifstream& readAllotmentShows(std::ifstream& inStream, MarketData& data) const;
    /**
     * @brief Reads spot market shows and willingness to pay from stream.
     * 
     * @param[out] inStream The stream to read the information from.
     * @param[out] data The MarketData to write the read information into.
     * @return The updated stream after reading.
     */
    std::ifstream& readSpotShows(std::ifstream& inStream, MarketData& data) const;
};

}   // namespace io
}   // namespace sea
