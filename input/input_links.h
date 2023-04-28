#pragma once

#include "../common.h"
#include "input_data.h"

#include <vector>
#include <utility>
#include <unordered_map>
#include <memory>
#include <map>

namespace sea {

using std::vector;
using std::pair;
using std::map;
using std::unordered_map;

struct InputLinks {
    vector<vector<ui32>> itinerariesFromArc; // indexed by arc-id
    vector<vector<ui32>> itinerariesWithArc; // indexed by arc-id
    vector<vector<ui32>> itinerariesToArc; // indexed by arc-id
    vector<vector<pair<ui32, ui32>>> demandTimesForItinerary; // indexed by id-itinerary, demandTimesForItinerary[id-itinerary] contains pairs <event_time, demand_index>
    vector<vector<ui32>> allotmentsWithItinerary; // indexed by id-itinerary
    vector<unordered_map<ui32, ui32>> allotmentItineraryToEntry; // indexed by [id-allotment][id-itinerary]
    vector<unordered_map<ui32, ui32>> allotmentItineraryToPlace; // for action and decision: allotmentContainersQ, allotmentDemandN, indexed by [id-allotment][id-itinerary]
    vector<vector<ui32>> allotmentToGroups;     // allotmentToGroups[i] -> list of groups with allotment i
    vector<ui32> firstArcToHireAfterArc; // indexed by arc-id
    vector<ui32> arrivalTime; // indexed by itinerary-id
};

using InputLinksPtr=std::shared_ptr<InputLinks>;
using InputLinksConstPtr=std::shared_ptr<const InputLinks>;

void createInputLinks(const InputData& input, InputLinks* links);
void printInputLinksStats(const InputLinks& links);

} // namespace sea
