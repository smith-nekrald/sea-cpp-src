// Implements API for creating InputLinks and related IO.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
// (c) Smith School of Business, 2025

#include "input_links.h"

#include <cassert>
#include <iostream>
#include <limits>


namespace sea {

using std::cout;
using std::endl;
using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;

void createInputLinks(const InputData& input, InputLinks* linksPtr) {
    const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();
    const double MAX_DOUBLE = std::numeric_limits<double>::max();

    assert(linksPtr != nullptr);
    InputLinks& links = *linksPtr;

    links.itinerariesToArc.resize(input.arcs.size());
    links.itinerariesToArc.shrink_to_fit();

    links.itinerariesFromArc.resize(input.arcs.size());
    links.itinerariesFromArc.shrink_to_fit();

    links.itinerariesWithArc.resize(input.arcs.size());
    links.itinerariesWithArc.shrink_to_fit();

    links.allotmentsWithItinerary.resize(input.itineraries.size());
    links.allotmentsWithItinerary.shrink_to_fit();
    links.demandTimesForItinerary.resize(input.itineraries.size());
    links.demandTimesForItinerary.shrink_to_fit();

    links.allotmentItineraryToEntry.resize(input.allotments.size());
    links.allotmentItineraryToEntry.shrink_to_fit();

    links.allotmentItineraryToPlace.resize(input.allotments.size());
    links.allotmentItineraryToPlace.shrink_to_fit();

    links.arrivalTime.assign(input.itineraries.size(), MAX_INDEX);
    links.arrivalTime.shrink_to_fit();
    links.firstArcToHireAfterArc.assign(input.arcs.size(), MAX_INDEX);
    links.firstArcToHireAfterArc.shrink_to_fit();

    for (const auto& itinerary : input.itineraries) {
        unsigned idArcStart = itinerary.orderedArcs.front();
        unsigned idArcEnd = itinerary.orderedArcs.back();
        for (unsigned idx = 0; idx < itinerary.orderedArcs.size(); ++idx) {
            unsigned idArcCur = itinerary.orderedArcs[idx];
            links.itinerariesWithArc[idArcCur].push_back(itinerary.id);
        }
        links.itinerariesToArc[idArcEnd].push_back(itinerary.id);
        links.itinerariesFromArc[idArcStart].push_back(itinerary.id);
    }

    vector<unsigned> pushStructure(input.allotments.size(), 0);
    for (const auto& allotment : input.allotments) {
        for (unsigned entryId : allotment.entries) {
            const auto& entry = input.allotmentEntries[entryId];
            unsigned idItinerary = entry.itinerary;
            links.allotmentItineraryToEntry[allotment.id][idItinerary] = entryId;
            links.allotmentItineraryToPlace[allotment.id][idItinerary] = pushStructure[allotment.id]++;
            links.allotmentsWithItinerary[idItinerary].push_back(allotment.id);
        }
    }

    vector<unsigned> waitingArc(input.ports.size(), MAX_INDEX);

    for (auto iterEvent = input.events.rbegin(); iterEvent != input.events.rend(); ++iterEvent) {
        if (iterEvent->type == EventType::cutoff) {
            unsigned portId = input.nodes[iterEvent->basedNode.value()].portId;
            waitingArc[portId] = iterEvent->basedArc.value();
        } else if (iterEvent->type == EventType::arrival) {
            unsigned portId = input.nodes[iterEvent->basedNode.value()].portId;
            links.firstArcToHireAfterArc[iterEvent->basedArc.value()] = waitingArc[portId];

        }
    }

    for (const auto& event : input.events) {
        if (event.type == EventType::pricing) {
            for (unsigned index = 0; index < event.demands.size(); ++index) {
                assert(event.demands.size() == event.relatedItineraryIds.size());
                unsigned idItinerary = event.relatedItineraryIds[index];
                links.demandTimesForItinerary[idItinerary].emplace_back(event.relativeTime, index);
            }
        } else if (event.type == EventType::arrival) {
            for (unsigned idItinerary : event.relatedItineraryIds) {
                links.arrivalTime[idItinerary] = event.relativeTime;
            }
        }
    }


    // Shrinking to fit and making unique

    for (unsigned idArc = 0; idArc < input.arcs.size(); ++idArc) {
        links.itinerariesToArc[idArc].shrink_to_fit();
        links.itinerariesFromArc[idArc].shrink_to_fit();
        links.itinerariesWithArc[idArc].shrink_to_fit();
    }

    for (unsigned idItinerary = 0; idItinerary < input.itineraries.size(); ++idItinerary) {
        links.allotmentsWithItinerary[idItinerary].shrink_to_fit();
    }

    links.allotmentToGroups.resize(input.allotments.size());
    for (unsigned groupId = 0; groupId < input.allotmentGroups.size(); ++groupId) {
        for (unsigned allotmentIndex : input.allotmentGroups[groupId]) {
            links.allotmentToGroups[allotmentIndex].push_back(groupId);
        }
    }
    for (auto & groupVector : links.allotmentToGroups) {
        groupVector.shrink_to_fit();
    }
    links.allotmentToGroups.shrink_to_fit();

    // Filling itineraryIdToCutoffDuration and itineraryIdToCutoffTime
    links.itineraryIdToCutoffDuration.assign(input.itineraries.size(), MAX_DOUBLE);
    links.itineraryIdToCutoffTime.assign(input.itineraries.size(), MAX_DOUBLE);
    for (const auto& event: input.events) {
        if (event.type == EventType::cutoff) {
            for (unsigned idItinerary : event.relatedItineraryIds) {
                links.itineraryIdToCutoffDuration[idItinerary] = event.duration;
                links.itineraryIdToCutoffTime[idItinerary] = event.realTime;
            }
        }
    }
    for (unsigned idItinerary = 0; idItinerary < input.itineraries.size(); ++idItinerary) {
        assert(links.itineraryIdToCutoffDuration[idItinerary] != MAX_DOUBLE);
        assert(links.itineraryIdToCutoffTime[idItinerary] != MAX_DOUBLE);
    }
    links.itineraryIdToCutoffDuration.shrink_to_fit();
    links.itineraryIdToCutoffTime.shrink_to_fit();
}

unsigned get2dVectorSize(const vector<vector<unsigned>>& target) {
    unsigned answer = 0;
    for (const auto& member : target) {
        answer += member.size();
    }
    return answer;
}

void printInputLinksStats(const InputLinks& links) {
    cout << "================================" << endl;
    cout << "InputLinks stats:" << endl;
    cout << "Amount of fields in itinerariesFromArc : " <<
        get2dVectorSize(links.itinerariesFromArc) << std::endl;
    cout << "Amount of fields in itinerariesWithArc : " <<
        get2dVectorSize(links.itinerariesWithArc) << std::endl;
    cout << "Amount of fields in itinerariesToArc : " <<
        get2dVectorSize(links.itinerariesToArc) << std::endl;
    cout << "Amount of fields in allotmentsWithItinerary : " <<
        get2dVectorSize(links.allotmentsWithItinerary) << std::endl;
    cout << "================================" << endl;
}

} // namespace sea
