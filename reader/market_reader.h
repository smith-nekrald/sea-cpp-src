#pragma once

#include <string>
#include <fstream>

#include "../input/market_data.h"
#include "ireader.h"

namespace sea {
namespace io {

class MarketReader : public IReader<MarketData> {
public:
    void Do(const std::string& filepath, MarketData& data) const override;

private:
    static const std::string filename;  // market.txt
    static const std::string header;    // MarketData:

    std::ifstream& readEvents(std::ifstream& inStream, MarketData& data) const;
    std::ifstream& readAllotmentShows(std::ifstream& inStream, MarketData& data) const;
    std::ifstream& readSpotShows(std::ifstream& inStream, MarketData& data) const;
};

}   // namespace io
}   // namespace sea
