// Implementation of methods and API mentioned in input_reader.h.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "input_reader.h"
#include "common_reader.hpp"
#include "../logging/logging.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <typeinfo>

namespace sea {
namespace io {

std::ifstream& validate(std::ifstream& stream, const std::string& header, unsigned& count) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);
    assert(tokens.size() >= 2);
    assert(tokens[0] == header);
    count = std::stoi(tokens[1]);
    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Port& port) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);

    port.id = std::stoi(tokens[3]);
    port.hiringCost = std::stof(tokens[6]);
    port.offHiringCost = std::stof(tokens[9]);
    port.loadCost = std::stof(tokens[12]);
    port.offLoadCost = std::stof(tokens[15]);
    port.storageCost = std::stof(tokens[18]);
    port.initialContainerCount = std::stoi(tokens[21]);
    port.finalContainerCount = std::stoi(tokens[27]);

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Node& node) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);

    node.id = std::stoi(tokens[3]);
    node.portId = std::stoi(tokens[6]);
    node.realTime = std::stof(tokens[9]);

    if (tokens[12] == "NodeType.arrival") {
        node.type = InputData::Node::Type::arrival;
    } else if (tokens[12] == "NodeType.departure") {
        node.type = InputData::Node::Type::departure;
    } else {
        throw std::runtime_error("no such NodeType: " + tokens[12]);
    }
    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Vessel& vessel) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);

    vessel.id = std::stoi(tokens[3]);
    vessel.capacity = std::stof(tokens[6]);
    vessel.speedAvg = std::stof(tokens[9]);

    unsigned wayLen = std::stoi(tokens[12]);

    std::vector<std::string> wayIds;
    makeTokens(stream, wayIds);

    for (unsigned idx = 0; idx < wayLen; ++idx) {
        vessel.way.push_back(std::stoi(wayIds[idx + 2]));
    }
    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Arc& arc) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);

    arc.id = std::stoi(tokens[3]);
    arc.fromNode = std::stoi(tokens[6]);
    arc.toNode = std::stoi(tokens[9]);

    if (tokens[12] == "ArcType.travel") {
        arc.type = InputData::Arc::Type::travel;
    } else if (tokens[12] == "ArcType.wait") {
        arc.type = InputData::Arc::Type::wait;
    } else if (tokens[12] == "ArcType.reload") {
        arc.type = InputData::Arc::Type::reload;
    } else {
        throw std::runtime_error("Unknown ArcType: " + tokens[12]);
    }

    if (tokens[15] != "None")
        arc.vesselId = std::stoi(tokens[15]);

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, ShowRate& show) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);
    assert((tokens[2] == "ShowRate:") || (tokens[2] == "RandomVariable:"));

    if (tokens.size() == 9) {
        show.estimatedProba = std::stof(tokens[8]);
    } else {
        show.estimatedProba = std::stof(tokens[11]);
    }
    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Itinerary& itinerary) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);

    itinerary.id = std::stoi(tokens[3]);
    itinerary.returnPrice = std::stof(tokens[6]);
    itinerary.preparedCapacity = std::stof(tokens[9]);
    itinerary.cost = std::stof(tokens[12]);
    itinerary.emptyCost = std::stof(tokens[15]);
    itinerary.declineCost = std::stof(tokens[18]);
    unsigned arcCount = std::stoi(tokens[21]);

    tokens.clear();
    makeTokens(stream, tokens);
    for (unsigned idx = 0; idx < arcCount; ++idx) {
        itinerary.orderedArcs.push_back(std::stoi(tokens[idx + 2]));
    }

    stream >> itinerary.showRate;
    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::AllotmentEntry& entry) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);

    entry.id = std::stoi(tokens[3]);
    entry.itinerary = std::stoi(tokens[6]);
    entry.productAmount = std::stoi(tokens[9]);
    entry.cancellationPrice = std::stof(tokens[12]);
    entry.price = std::stof(tokens[15]);

    stream >> entry.showRate;
    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Allotment& allotment) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);

    allotment.id = std::stoi(tokens[3]);

    unsigned entryCount = std::stoi(tokens[6]);

    tokens.clear();
    makeTokens(stream, tokens);

    for (unsigned idx = 0; idx < entryCount; ++idx) {
        allotment.entries.push_back(std::stoi(tokens[idx + 2]));
    }

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, Demand& demand) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);

    if (tokens[8] == "DemandType.exponential") {
        demand.type = Demand::Type::exponential;
    } else if (tokens[8] == "DemandType.linear") {
        demand.type = Demand::Type::linear;
    } else {
        throw std::runtime_error("Unknown DemandType: " + tokens[8]);
    }

    if (demand.type == Demand::Type::exponential) {
        demand.scale = std::stof(tokens[11]);
        demand.sensitivity = std::stof(tokens[14]);
    } else if (demand.type == Demand::Type::linear) {
        demand.multiplicative = std::stof(tokens[11]);
        demand.additive = std::stof(tokens[14]);
    }

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Event& event) {
    std::vector<std::string> tokens;
    makeTokens(stream, tokens);

    if (tokens[3] == "EventType.pricing") {
        event.type = InputData::Event::Type::pricing;
    } else if (tokens[3] == "EventType.cutoff") {
        event.type = InputData::Event::Type::cutoff;
    } else if (tokens[3] == "EventType.arrival") {
        event.type = InputData::Event::Type::arrival;
    } else {
        throw std::runtime_error("Unknown EventType: " + tokens[3]);
    }

    event.realTime = std::stof(tokens[6]);
    event.relativeTime = std::stof(tokens[9]);
    event.duration = std::stof(tokens[12]);

    if (tokens[15] != "None") {
        event.basedNode = std::stoi(tokens[15]);
    }
    if (tokens[18] != "None") {
        event.basedArc = std::stoi(tokens[18]);
    }

    unsigned relatedItineraryCount = std::stoi(tokens[21]);
    unsigned demandsCount = std::stoi(tokens[24]);

    tokens.clear();
    makeTokens(stream, tokens);

    for (unsigned ind = 0; ind < relatedItineraryCount; ++ind) {
        event.relatedItineraryIds.push_back(std::stoi(tokens[ind + 2]));
    }

    for (unsigned idx = 0; idx < demandsCount; ++idx) {
        Demand demand;
        stream >> demand;
        event.demands.push_back(std::move(demand));
    }

    return stream;
}

std::ifstream& InputReader::readPorts(std::ifstream& inStream, InputData& data) const {
    return read(inStream, "Ports:", data.ports);
}

std::ifstream& InputReader::readNodes(std::ifstream& inStream, InputData& data) const {
    return read(inStream, "Nodes:", data.nodes);
}

std::ifstream& InputReader::readVessels(std::ifstream& inStream, InputData& data) const {
    return read(inStream, "Vessels:", data.vessels);
}

std::ifstream& InputReader::readArcs(std::ifstream& inStream, InputData& data) const {
    return read(inStream, "Arcs:", data.arcs);
}

std::ifstream& InputReader::readItineraries(std::ifstream& inStream, InputData& data) const {
    return read(inStream, "Itineraries:", data.itineraries);
}

std::ifstream& InputReader::readAllotmentEntries(std::ifstream& inStream, InputData& data) const {
    return read(inStream, "AllotmentEntries:", data.allotmentEntries);
}

std::ifstream& InputReader::readAllotments(std::ifstream& inStream, InputData& data) const {
    return read(inStream, "Allotments:", data.allotments);
}

std::ifstream& InputReader::readEvents(std::ifstream& inStream, InputData& data) const {
    unsigned count = 0;
    validate(inStream, "Events:", count);
    for (unsigned idx = 0; idx < count; ++idx) {
        InputData::Event event;
        inStream >> event;
        data.events.push_back(std::move(event));
    }
    std::sort(std::begin(data.events), std::end(data.events),
        [] (const InputData::Event& lhs, const InputData::Event& rhs) {
            return lhs.relativeTime < rhs.relativeTime;
    });
    return inStream;
}

std::ifstream& InputReader::readGroups(std::ifstream& inStream, InputData& data) const {
    unsigned count = 0;
    validate(inStream, "SelectOneGroups:", count);
    for (unsigned idx = 0; idx < count; ++idx) {
        data.allotmentGroups.emplace_back();
        makeTokens<unsigned>(inStream, data.allotmentGroups.back());
    }
    return inStream;
}

std::ifstream& InputReader::readLeaseCost(std::ifstream& inStream, InputData& data) const {
    std::vector<std::string> tokens;
    makeTokens(inStream, tokens);
    assert(tokens[0] == "LeaseCost:");
    data.leaseCost = std::stof(tokens[1]);
    return inStream;
}

const std::string InputReader::header = "InputData:";

void InputReader::Do(const std::string& filepath, InputData& data) const {
    std::ifstream inStream(filepath, std::ifstream::in);
    if (!inStream.good()) {
        throw std::runtime_error("Failed to open input file: " + filepath);
    }

    std::vector<std::string> tokens;
    makeTokens(inStream, tokens);
    assert(tokens[0] == header);

    readPorts(inStream, data);
    readNodes(inStream, data);
    readVessels(inStream, data);
    readArcs(inStream, data);
    readItineraries(inStream, data);
    readAllotmentEntries(inStream, data);
    readAllotments(inStream, data);
    readEvents(inStream, data);
    readGroups(inStream, data);
    readLeaseCost(inStream, data);
}

}   // namespace io
}   // namespace sea

