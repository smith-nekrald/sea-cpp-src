#include "input_links.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>

using std::cout;
using std::endl;

const ui32 MAX_INDEX = std::numeric_limits<ui32>::max();

namespace sea {

using std::sort;
using std::unique;
using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;

void createInputLinks(const InputData& input, InputLinks* linksPtr) {
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
        ui32 idArcStart = itinerary.orderedArcs.front();
        ui32 idArcEnd = itinerary.orderedArcs.back();
        for (ui32 i = 0; i < itinerary.orderedArcs.size(); ++i) {
            ui32 idArcCur = itinerary.orderedArcs[i];
            links.itinerariesWithArc[idArcCur].push_back(itinerary.id);
        }
        links.itinerariesToArc[idArcEnd].push_back(itinerary.id);
        links.itinerariesFromArc[idArcStart].push_back(itinerary.id);
    }

    vector<ui32> pushStructure(input.allotments.size(), 0);
    for (const auto& allotment : input.allotments) {
        for (ui32 entryId : allotment.entries) {
            const auto& entry = input.allotmentEntries[entryId];
            ui32 idItinerary = entry.itinerary;
            links.allotmentItineraryToEntry[allotment.id][idItinerary] = entryId;
            links.allotmentItineraryToPlace[allotment.id][idItinerary] = pushStructure[allotment.id]++;
            links.allotmentsWithItinerary[idItinerary].push_back(allotment.id);
        }
    }

    vector<ui32> waitingArc(input.ports.size(), MAX_INDEX);

    for (auto it = input.events.rbegin(); it != input.events.rend(); ++it) {
        if (it->type == EventType::cutoff) {
            ui32 portId = input.nodes[it->basedNode.value()].portId;
            waitingArc[portId] = it->basedArc.value();
        } else if (it->type == EventType::arrival) {
            ui32 portId = input.nodes[it->basedNode.value()].portId;
            links.firstArcToHireAfterArc[it->basedArc.value()] =
                waitingArc[portId];

        }
    }

    for (const auto& event : input.events) {
        if (event.type == EventType::pricing) {
            for (ui32 index = 0; index < event.demands.size(); ++index) {
                assert(event.demands.size() == event.relatedItineraryIds.size());
                ui32 idItinerary = event.relatedItineraryIds[index];
                links.demandTimesForItinerary[idItinerary].emplace_back(event.relativeTime, index);
            }
        } else if (event.type == EventType::arrival) {
            for (ui32 idItinerary : event.relatedItineraryIds) {
                links.arrivalTime[idItinerary] = event.relativeTime;
            }
        }
    }


    // Shrinking to fit and making unique

    for (ui32 idArc = 0; idArc < input.arcs.size(); ++idArc) {
        links.itinerariesToArc[idArc].shrink_to_fit();
        links.itinerariesFromArc[idArc].shrink_to_fit();
        links.itinerariesWithArc[idArc].shrink_to_fit();
    }

    for (ui32 idItinerary = 0; idItinerary < input.itineraries.size(); ++idItinerary) {
        links.allotmentsWithItinerary[idItinerary].shrink_to_fit();
    }

    links.allotmentToGroups.resize(input.allotments.size());
    for (ui32 groupId = 0; groupId < input.allotmentGroups.size(); ++groupId) {
        for (ui32 allotmentIndex : input.allotmentGroups[groupId]) {
            links.allotmentToGroups[allotmentIndex].push_back(groupId);
        }
    }
    for (auto & vec : links.allotmentToGroups) {
        vec.shrink_to_fit();
    }
    links.allotmentToGroups.shrink_to_fit();
}

ui32 get2dVectorSize(const vector<vector<ui32>>& target) {
    ui32 ans = 0;
    for (const auto& member : target) {
        ans += member.size();
    }
    return ans;
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
