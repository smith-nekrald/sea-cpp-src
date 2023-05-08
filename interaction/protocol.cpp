#include "protocol.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <exception>
#include <sstream>

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/base_class.hpp>

namespace cereal {

template<class Archive>
void serialize(Archive& ar, sea::Decision& decision)
{
    ar(decision.time,
        decision.offHiredInPortS,
        decision.prices,

        decision.emptyContainersZ,
        decision.nonEmptyContainersQ,
        decision.allotmentContainersQ,

        decision.hiredY,

        decision.allotmentAccepted);
}

template<class Archive>
void serialize(Archive& ar, sea::Action& action)
{
    ar(action.time,
        action.spotMarketDemandN,
        action.bookingsB,

        action.allotmentDemandN);
}

} // namespace cereal


using std::endl;
using std::vector;
using std::size_t;

namespace sea {

std::ostream& operator<<(std::ostream& out, const Decision& decision) {
    writeToStream(out, decision);
    return out;
}

void writeToFile(const std::string& pathToFile, const Decision& decision) {
    std::ofstream writer(pathToFile, std::ios::binary);
    ::cereal::BinaryOutputArchive oa(writer);
    oa << decision;
}

void loadFromFile(const std::string& pathToFile, Decision* emptyDecision) {
    std::ifstream reader(pathToFile, std::ios::binary);
    ::cereal::BinaryInputArchive ia(reader);
    ia >> *emptyDecision;
}

void createDecision(const InputData& input, Decision* decision) {
    decision->offHiredInPortS.resize(input.events.size());
    decision->offHiredInPortS.shrink_to_fit();

    decision->prices.resize(input.events.size());
    decision->prices.shrink_to_fit();

    for (const auto& event : input.events) {
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned int idItinerary : event.relatedItineraryIds) {
                decision->prices[event.relativeTime].push_back(std::make_pair(idItinerary, 0));
            }
        }
        decision->prices[event.relativeTime].shrink_to_fit();
        decision->offHiredInPortS[event.relativeTime].assign(input.ports.size(), 0);
        decision->offHiredInPortS[event.relativeTime].shrink_to_fit();
    }

    decision->emptyContainersZ.assign(input.itineraries.size(), 0);
    decision->emptyContainersZ.shrink_to_fit();

    decision->nonEmptyContainersQ.assign(input.itineraries.size(), 0);
    decision->nonEmptyContainersQ.shrink_to_fit();

    decision->hiredY.assign(input.arcs.size(), 0);
    decision->hiredY.shrink_to_fit();

    decision->allotmentAccepted.assign(input.allotments.size(), false);
    decision->allotmentAccepted.shrink_to_fit();

    decision->allotmentContainersQ.resize(input.allotments.size());
    decision->allotmentContainersQ.shrink_to_fit();

    for (const auto& allotment : input.allotments) {
        for (unsigned int entryId : allotment.entries) {
            const auto& entry = input.allotmentEntries[entryId];
            unsigned int idItinerary = entry.itinerary;
            decision->allotmentContainersQ[allotment.id].push_back(
                std::make_pair(idItinerary, (unsigned int)(0)));
        }
    }

    for (unsigned int idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        decision->allotmentContainersQ[idAllotment].shrink_to_fit();
    }
}

void createAction(const InputData& input, Action* action) {
    action->bookingsB.resize(input.events.size());
    action->bookingsB.shrink_to_fit();
    for (const auto& event : input.events) {
        if (event.type == InputData::Event::Type::pricing) {
            action->bookingsB[event.relativeTime].assign(input.itineraries.size(), 0);
            action->bookingsB[event.relativeTime].shrink_to_fit();
        }
    }

    action->spotMarketDemandN.assign(input.itineraries.size(), 0);
    action->spotMarketDemandN.shrink_to_fit();

    action->allotmentDemandN.resize(input.allotments.size());
    action->allotmentDemandN.shrink_to_fit();

    for (const auto& allotment : input.allotments) {
        for (unsigned int entryId : allotment.entries) {
            const auto& entry = input.allotmentEntries[entryId];
            unsigned int idItinerary = entry.itinerary;
            action->allotmentDemandN[allotment.id].push_back(
                std::make_pair(idItinerary, (unsigned int)(0)));
        }
    }

    for (unsigned int idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        action->allotmentDemandN[idAllotment].shrink_to_fit();
    }
}
std::ostream& operator<<(std::ostream& out, const Action& action) {
    writeToStream(out, action);
    return out;
}

void writeToFile(const std::string& pathToFile, const Action& filledAction) {
    std::ofstream writer(pathToFile, std::ios::binary);
    ::cereal::BinaryOutputArchive oa(writer);
    oa << filledAction;
}

void loadFromFile(const std::string& pathToFile, Action* emptyAction) {
    std::ifstream reader(pathToFile, std::ios::binary);
    ::cereal::BinaryInputArchive ia(reader);
    ia >> *emptyAction;
}


} // namespace sea
