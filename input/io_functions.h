#pragma once

#include "input_data.h"
#include "market_data.h"
#include "input_links.h"

#include <string>

namespace sea {

void loadFromFile(const std::string& pathToFile, InputData* data);
void writeToFile(const std::string& pathToFile, const InputData& data);

void loadFromFile(const std::string& pathToFile, MarketData* data);
void writeToFile(const std::string& pathToFile, const MarketData& data);

void loadFromFile(const std::string& pathToFile, InputLinks* links);
void writeToFile(const std::string& pathToFile, const InputLinks& links);

} // namespace sea
