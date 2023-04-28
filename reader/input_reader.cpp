#include "input_reader.h"
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

namespace {

template <typename T = std::string>
inline void make_tokens(const std::string& s, std::vector<T>& tokens) {

    std::istringstream iss(s);

    std::vector<T> tmp{std::istream_iterator<T>{iss}, std::istream_iterator<T>{}};

    tokens.swap(tmp);
}

template <typename T = std::string>
inline std::ifstream& make_tokens(std::ifstream& stream, std::vector<T>& tokens) {

    std::string to_parse;
    while (to_parse.empty())
        std::getline(stream, to_parse);

    make_tokens<T>(to_parse, tokens);

    return stream;
}

}   // namespace

inline std::ifstream& validate(std::ifstream& stream, const std::string& header, ui32& count) {

    std::vector<std::string> tokens;
    make_tokens(stream, tokens);

    assert(tokens.size() >= 2);
    assert(tokens[0] == header);

    count = std::stoi(tokens[1]);

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Port& port) {

    std::vector<std::string> tokens;
    make_tokens(stream, tokens);

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
    make_tokens(stream, tokens);

    node.id = std::stoi(tokens[3]);
    node.portId = std::stoi(tokens[6]);
    node.realTime = std::stof(tokens[9]);

    if (tokens[12] == "NodeType.arrival")
        node.type = InputData::Node::Type::arrival;
    else if (tokens[12] == "NodeType.departure")
        node.type = InputData::Node::Type::departure;
    else
        throw std::runtime_error("no such NodeType: " + tokens[12]);

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Vessel& vessel) {

    std::vector<std::string> tokens;
    make_tokens(stream, tokens);

    vessel.id = std::stoi(tokens[3]);
    vessel.capacity = std::stof(tokens[6]);
    vessel.speedAvg = std::stof(tokens[9]);

    ui32 wayLen = std::stoi(tokens[12]);

    std::vector<std::string> wayIds;
    make_tokens(stream, wayIds);

    for (ui32 i = 0; i < wayLen; ++i)
        vessel.way.push_back(std::stoi(wayIds[i + 2]));

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Arc& arc) {

    std::vector<std::string> tokens;
    make_tokens(stream, tokens);

    arc.id = std::stoi(tokens[3]);
    arc.fromNode = std::stoi(tokens[6]);
    arc.toNode = std::stoi(tokens[9]);

    if (tokens[12] == "ArcType.travel")
        arc.type = InputData::Arc::Type::travel;
    else if (tokens[12] == "ArcType.wait")
        arc.type = InputData::Arc::Type::wait;
    else if (tokens[12] == "ArcType.reload")
        arc.type = InputData::Arc::Type::reload;
    else
        throw std::runtime_error("no such ArcType: " + tokens[12]);

    if (tokens[15] != "None")
        arc.vesselId = std::stoi(tokens[15]);

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, ShowRate& s) {

    std::vector<std::string> tokens;
    make_tokens(stream, tokens);

    assert((tokens[2] == "ShowRate:") || (tokens[2] == "RandomVariable:"));

    if (tokens.size() == 9)
        s.estimatedProba = std::stof(tokens[8]);
    else
        s.estimatedProba = std::stof(tokens[11]);

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::Itinerary& it) {

    std::vector<std::string> tokens;
    make_tokens(stream, tokens);

    it.id = std::stoi(tokens[3]);
    it.returnPrice = std::stof(tokens[6]);
    it.preparedCapacity = std::stof(tokens[9]);
    it.cost = std::stof(tokens[12]);
    it.emptyCost = std::stof(tokens[15]);
    it.declineCost = std::stof(tokens[18]);

    ui32 arcCount = std::stoi(tokens[21]);

    tokens.clear();
    make_tokens(stream, tokens);

    for (ui32 i = 0; i < arcCount; ++i)
        it.orderedArcs.push_back(std::stoi(tokens[i + 2]));

    stream >> it.showRate;

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, InputData::AllotmentEntry& entry) {

    std::vector<std::string> tokens;
    make_tokens(stream, tokens);

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
    make_tokens(stream, tokens);

    allotment.id = std::stoi(tokens[3]);

    ui32 entryCount = std::stoi(tokens[6]);

    tokens.clear();
    make_tokens(stream, tokens);

    for (ui32 i = 0; i < entryCount; ++i)
        allotment.entries.push_back(std::stoi(tokens[i + 2]));

    return stream;
}

std::ifstream& operator>>(std::ifstream& stream, Demand& demand) {

    std::vector<std::string> tokens;
    make_tokens(stream, tokens);

    if (tokens[8] == "DemandType.exponential")
        demand.type = Demand::Type::exponential;
    else if (tokens[8] == "DemandType.linear")
        demand.type = Demand::Type::linear;
    else
        throw std::runtime_error("no such DemandType: " + tokens[8]);

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
    make_tokens(stream, tokens);

    if (tokens[3] == "EventType.pricing")
        event.type = InputData::Event::Type::pricing;
    else if (tokens[3] == "EventType.cutoff")
        event.type = InputData::Event::Type::cutoff;
    else if (tokens[3] == "EventType.arrival")
        event.type = InputData::Event::Type::arrival;
    else
        throw std::runtime_error("no such EventType: " + tokens[3]);

    event.realTime = std::stof(tokens[6]);
    event.relativeTime = std::stof(tokens[9]);
    event.duration = std::stof(tokens[12]);

    if (tokens[15] != "None")
        event.basedNode = std::stoi(tokens[15]);
    if (tokens[18] != "None")
        event.basedArc = std::stoi(tokens[18]);

    ui32 relatedItineraryCount = std::stoi(tokens[21]);
    ui32 demandsCount = std::stoi(tokens[24]);

    tokens.clear();
    make_tokens(stream, tokens);

    for (ui32 i = 0; i < relatedItineraryCount; ++i)
        event.relatedItineraryIds.push_back(std::stoi(tokens[i + 2]));

    for (ui32 i = 0; i < demandsCount; ++i) {
        Demand d;
        stream >> d;
        event.demands.push_back(std::move(d));
    }

    return stream;
}

template <typename T, typename Stream = std::ifstream>
inline std::ifstream& read(Stream& stream, const std::string& header, std::vector<T>& data) {
    ui32 count = 0;
    validate(stream, header, count);

    for (ui32 i = 0; i < count; ++i) {
        T t;
        stream >> t;
        data.push_back(std::move(t));
    }

    std::sort(std::begin(data), std::end(data), [] (const T& l, const T& r) {return l.id < r.id;});

    return stream;
}

std::ifstream& InputReader::readPorts(std::ifstream& ifs, InputData& data) const {
    return read(ifs, "Ports:", data.ports);
}

std::ifstream& InputReader::readNodes(std::ifstream& ifs, InputData& data) const {
    return read(ifs, "Nodes:", data.nodes);
}

std::ifstream& InputReader::readVessels(std::ifstream& ifs, InputData& data) const {
    return read(ifs, "Vessels:", data.vessels);
}

std::ifstream& InputReader::readArcs(std::ifstream& ifs, InputData& data) const {
    return read(ifs, "Arcs:", data.arcs);
}

std::ifstream& InputReader::readItineraries(std::ifstream& ifs, InputData& data) const {
    return read(ifs, "Itineraries:", data.itineraries);
}

std::ifstream& InputReader::readAllotmentEntries(std::ifstream& ifs, InputData& data) const {
    return read(ifs, "AllotmentEntries:", data.allotmentEntries);
}

std::ifstream& InputReader::readAllotments(std::ifstream& ifs, InputData& data) const {
    return read(ifs, "Allotments:", data.allotments);
}

std::ifstream& InputReader::readEvents(std::ifstream& ifs, InputData& data) const {
    ui32 count = 0;
    validate(ifs, "Events:", count);

    for (ui32 i = 0; i < count; ++i) {
        InputData::Event e;
        ifs >> e;
        data.events.push_back(std::move(e));
    }

    std::sort(std::begin(data.events), std::end(data.events),
        [] (const InputData::Event& l, const InputData::Event& r) {
            return l.relativeTime < r.relativeTime;
    });

    return ifs;
}

std::ifstream& InputReader::readGroups(std::ifstream& ifs, InputData& data) const {
    ui32 count = 0;
    validate(ifs, "SelectOneGroups:", count);

    for (ui32 i = 0; i < count; ++i) {
        data.allotmentGroups.emplace_back();
        make_tokens<ui32>(ifs, data.allotmentGroups.back());
    }

    return ifs;
}

std::ifstream& InputReader::readLeaseCost(std::ifstream& ifs, InputData& data) const {
    std::vector<std::string> tokens;
    make_tokens(ifs, tokens);

    assert(tokens[0] == "LeaseCost:");

    data.leaseCost = std::stof(tokens[1]);

    return ifs;
}

const std::string InputReader::header = "InputData:";

void InputReader::Do(const std::string& filepath, InputData& data) const {
    std::ifstream ifs(filepath, std::ifstream::in);

    if (!ifs.good())
        throw std::runtime_error("failed to open input file: " + filepath);

    logging::getRootLogger().debug("Ready to read in InputReader::Do, file is " + filepath);

    std::vector<std::string> tokens;
    make_tokens(ifs, tokens);
    logging::getRootLogger().debug("Finished make_tokens.");
    assert(tokens[0] == header);

    readPorts(ifs, data);
    logging::getRootLogger().debug("Finished readPorts.");

    readNodes(ifs, data);
    logging::getRootLogger().debug("Finished readNodes.");

    readVessels(ifs, data);
    logging::getRootLogger().debug("Finished readVessels.");

    readArcs(ifs, data);
    logging::getRootLogger().debug("Finished readArcs.");

    readItineraries(ifs, data);
    logging::getRootLogger().debug("Finished readItineraries.");

    readAllotmentEntries(ifs, data);
    logging::getRootLogger().debug("Finished readAllotmentsEntries.");

    readAllotments(ifs, data);
    logging::getRootLogger().debug("Finished readAllotments.");

    readEvents(ifs, data);
    logging::getRootLogger().debug("Finished readEvents.");

    readGroups(ifs, data);
    logging::getRootLogger().debug("Finished readGroups.");

    readLeaseCost(ifs, data);
    logging::getRootLogger().debug("Finished readLeaseCost.");
}

}   // namespace io
}   // namespace sea




