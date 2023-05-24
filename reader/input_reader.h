/**
 * @file input_reader.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief InputReader is an entity to read InputData from file.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <string>
#include "../input/input_data.h"
#include "ireader.h"

namespace sea {
namespace io {

/**
 * @brief Entity to read InputData.
 */
class InputReader : public IReader<InputData>  {
public:
    /**
     * @brief Main entry point to read input data from file.
     * 
     * @param[in] filepath Path to the file with stored InputData.
     * @param[out] data The object to write the InputData into.
     */
    void Do(const std::string& filepath, InputData& data) const override;
    /**
     * @brief Virtual destructor. To ensure that C++ polymorphism works correctly.
     */
    virtual ~InputReader() {}

protected:
    /// @brief The header at the Input Data top. Should coinside with "InputData".
    static const std::string header;

protected:
    /**
     * @brief Reads ports from a stream.
     * 
     * @param[out] inStream Stream to read the ports from.
     * @param[out] data The InputData structure to write the read ports into.
     * @return The updated stream after reading ports.
     */
    std::ifstream& readPorts(std::ifstream& inStream, InputData& data) const;
    /**
     * @brief Reads nodes from a stream.
     * 
     * @param[out] inStream Stream to read the nodes from.
     * @param[out] data The InputData structure to write the read nodes into.
     * @return The updated stream after reading nodes.
     */
    std::ifstream& readNodes(std::ifstream& inStream, InputData& data) const;
    /**
     * @brief Reads vessels from a stream.
     * 
     * @param[out] inStream Stream to read the vessels from.
     * @param[out] data The InputData structure to write the read vessels into.
     * @return The updated stream after reading vessels.
     */
    std::ifstream& readVessels(std::ifstream& inStream, InputData& data) const;
    /**
     * @brief Reads arcs from a stream.
     * 
     * @param[out] inStream Stream to read the arcs from.
     * @param[out] data The InputData structure to write the read arcs into.
     * @return The updated stream after reading arcs.
     */
    std::ifstream& readArcs(std::ifstream& inStream, InputData& data) const;
    /**
     * @brief Reads itineraries from a stream.
     * 
     * @param[out] inStream The stream to read itineraries from.
     * @param[out] data The InputData structure to write the read itineraries into.
     * @return The updated stream after reading itineraries.
     */
    std::ifstream& readItineraries(std::ifstream& inStream, InputData& data) const;
    /**
     * @brief Reads allotment entries from a stream.
     * 
     * @param[out] inStream The stream to read allotment entries from.
     * @param[out] data The InputData structure to write the read allotment entires into.
     * @return The updated stream after reading allotment entries.
     */
    std::ifstream& readAllotmentEntries(std::ifstream& inStream, InputData& data) const;
    /**
     * @brief Reads allotments from a stream.
     * 
     * @param[out] inStream The stream to read allotments from.
     * @param[out] data The InputData structure to write the read allotments into.
     * @return The updated stream after reading allotments.
     */
    std::ifstream& readAllotments(std::ifstream& inStream, InputData& data) const;
    /**
     * @brief Reads events from a stream.
     * 
     * @param[out] inStream The stream to read events from.
     * @param[out] data The InputData structure to write the read events into.
     * @return The updated stream after reading events.
     */
    std::ifstream& readEvents(std::ifstream& inStream, InputData& data) const;
    /**
     * @brief Reads one-allotment-groups from a stream.
     * 
     * @param[out] inStream The stream to read groups from.
     * @param[out] data The InputData structure to write the read groups into.
     * @return The updated stream after reading groups.
     */
    std::ifstream& readGroups(std::ifstream& inStream, InputData& data) const;
    /**
     * @brief Reads lease cost from a stream.
     * 
     * @param[out] inStream The stream to read lease cost from.
     * @param[out] data The InputData structure to write the read lease cost into.
     * @return The updated stream after reading lease cost.
     */
    std::ifstream& readLeaseCost(std::ifstream& inStream, InputData& data) const;
};

/**
 * @brief Reads the count of next items for reading and verifies header.
 * 
 * @param[out] stream The stream to read info from.
 * @param[in] header The header to verify.
 * @param[out] count The count to read.
 * @return The updated stream after reading.
 */
std::ifstream& validate(std::ifstream& stream, const std::string& header, unsigned& count);
/**
 * @brief Reads a port from stream.
 * 
 * @param[out] stream Stream to read the port from.
 * @param[out] port The port to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, InputData::Port& port);
/**
 * @brief Reads a node from stream.
 * 
 * @param[out] stream The stream to read the node from.
 * @param[out] node The node to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, InputData::Node& node);
/**
 * @brief Reads vessel from stream.
 * 
 * @param[out] stream The stream to read from.
 * @param[out] vessel The vessel to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, InputData::Vessel& vessel);
/**
 * @brief Reads arc from stream.
 * 
 * @param[out] stream The stream to read from.
 * @param[out] arc The arc to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, InputData::Arc& arc);
/**
 * @brief Reads show rate from stream.
 * 
 * @param[out] stream The stream to read from.
 * @param[out] show  The show rate to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, ShowRate& show);
/**
 * @brief Reads itinerary from stream.
 * 
 * @param[out] stream The stream to read from.
 * @param[out] itinerary The itinerary to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, InputData::Itinerary& itinerary);
/**
 * @brief Reads allotment entry from stream.
 * 
 * @param[out] stream The stream to read from.
 * @param[out] entry The allotment entry to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, InputData::AllotmentEntry& entry);
/**
 * @brief Reads allotment from stream.
 * 
 * @param[out] stream The stream to read from.
 * @param[out] allotment The allotment to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, InputData::Allotment& allotment);
/**
 * @brief Reads demand from stream.
 * 
 * @param[out] stream The stream to read from.
 * @param[out] demand The demand to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, Demand& demand);
/**
 * @brief Reads event from stream.
 * 
 * @param[out] stream The stream to read from.
 * @param[out] event The event to read.
 * @return Updated stream for further applications.
 */
std::ifstream& operator>>(std::ifstream& stream, InputData::Event& event);

}   // namespace io
}   // namespace sea
