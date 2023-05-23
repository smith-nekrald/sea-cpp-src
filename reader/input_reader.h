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

    std::ifstream& readPorts(std::ifstream& inStream, InputData& data) const;
    std::ifstream& readNodes(std::ifstream& inStream, InputData& data) const;
    std::ifstream& readVessels(std::ifstream& inStream, InputData& data) const;
    std::ifstream& readArcs(std::ifstream& inStream, InputData& data) const;
    std::ifstream& readItineraries(std::ifstream& inStream, InputData& data) const;
    std::ifstream& readAllotmentEntries(std::ifstream& inStream, InputData& data) const;
    std::ifstream& readAllotments(std::ifstream& inStream, InputData& data) const;
    std::ifstream& readEvents(std::ifstream& inStream, InputData& data) const;
    std::ifstream& readGroups(std::ifstream& inStream, InputData& data) const;
    std::ifstream& readLeaseCost(std::ifstream& inStream, InputData& data) const;
};

std::ifstream& validate(std::ifstream& stream, const std::string& header, unsigned& count);
std::ifstream& operator>>(std::ifstream& stream, InputData::Port& port);
std::ifstream& operator>>(std::ifstream& stream, InputData::Node& node);
std::ifstream& operator>>(std::ifstream& stream, InputData::Vessel& vessel);
std::ifstream& operator>>(std::ifstream& stream, InputData::Arc& arc);
std::ifstream& operator>>(std::ifstream& stream, ShowRate& show);
std::ifstream& operator>>(std::ifstream& stream, InputData::Itinerary& itinerary);
std::ifstream& operator>>(std::ifstream& stream, InputData::AllotmentEntry& entry);
std::ifstream& operator>>(std::ifstream& stream, InputData::Allotment& allotment);
std::ifstream& operator>>(std::ifstream& stream, Demand& demand);
std::ifstream& operator>>(std::ifstream& stream, InputData::Event& event);

}   // namespace io
}   // namespace sea
