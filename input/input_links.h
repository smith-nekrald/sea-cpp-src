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
    vector<vector<unsigned>> itinerariesFromArc; // indexed by arc-id
    vector<vector<unsigned>> itinerariesWithArc; // indexed by arc-id
    vector<vector<unsigned>> itinerariesToArc; // indexed by arc-id
    vector<vector<pair<unsigned, unsigned>>> demandTimesForItinerary; // indexed by id-itinerary, demandTimesForItinerary[id-itinerary] contains pairs <event_time, demand_index>
    vector<vector<unsigned>> allotmentsWithItinerary; // indexed by id-itinerary
    vector<unordered_map<unsigned, unsigned>> allotmentItineraryToEntry; // indexed by [id-allotment][id-itinerary]
    vector<unordered_map<unsigned, unsigned>> allotmentItineraryToPlace; // for action and decision: allotmentContainersQ, allotmentDemandN, indexed by [id-allotment][id-itinerary]
    vector<vector<unsigned>> allotmentToGroups;     // allotmentToGroups[i] -> list of groups with allotment i
    vector<unsigned> firstArcToHireAfterArc; // indexed by arc-id
    vector<unsigned> arrivalTime; // indexed by itinerary-id
};

using InputLinksPtr=std::shared_ptr<InputLinks>;
using InputLinksConstPtr=std::shared_ptr<const InputLinks>;

void createInputLinks(const InputData& input, InputLinks* links);
void printInputLinksStats(const InputLinks& links);

} // namespace sea
