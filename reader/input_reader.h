#pragma once

#include <string>
#include "../input/input_data.h"
#include "ireader.h"

namespace sea {
namespace io {

class InputReader : public IReader<InputData>  {
public:
    void Do(const std::string& filepath, InputData& data) const override;

protected:
    static const std::string filename;  // "input.txt"
    static const std::string header;    // InputData:

    std::ifstream& readPorts(std::ifstream& ifs, InputData& data) const;
    std::ifstream& readNodes(std::ifstream& ifs, InputData& data) const;
    std::ifstream& readVessels(std::ifstream& ifs, InputData& data) const;
    std::ifstream& readArcs(std::ifstream& ifs, InputData& data) const;
    std::ifstream& readItineraries(std::ifstream& ifs, InputData& data) const;
    std::ifstream& readAllotmentEntries(std::ifstream& ifs, InputData& data) const;
    std::ifstream& readAllotments(std::ifstream& ifs, InputData& data) const;
    std::ifstream& readEvents(std::ifstream& ifs, InputData& data) const;
    std::ifstream& readGroups(std::ifstream& ifs, InputData& data) const;
    std::ifstream& readLeaseCost(std::ifstream& ifs, InputData& data) const;
};

}   // namespace io
}   // namespace sea
