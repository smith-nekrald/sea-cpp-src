// Read-Write functions for InputData, InputLinks and MarketData.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "io_functions.h"
#include "../reader/input_reader.h"
#include "../reader/market_reader.h"
#include "io_functions.hpp"


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
    ::cereal::BinaryInputArchive inputArchive(reader);
    inputArchive >> *links;
}

void writeToFile(const std::string& pathToFile, const InputLinks& links) {
    std::ofstream writer(pathToFile, std::ios::binary);
    ::cereal::BinaryOutputArchive outputArchive(writer);
    outputArchive << links;
}

} // namespace sea
