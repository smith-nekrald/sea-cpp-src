/**
 * @file input_links.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines InputLinks and corresponding API. InputLinks is a structure over InputData
 * for easier and faster field access and processing.
 * @copyright (c) Smith School of Business, 2023
 * @copyright (c) Smith School of Business, 2025
 */
#pragma once

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

/// @brief InputLinks is a structure over InputData for easier access to the fields in InputData.
struct InputLinks {
    /// @brief Precomputes indices of itineraries starting from a particular arc. Indexed by arc-id.
    vector<vector<unsigned>> itinerariesFromArc;
    /// @brief Precomputes indices of itineraries going through a particular arc. Indexed by arc-id.
    vector<vector<unsigned>> itinerariesWithArc;
    /// @brief Precomputes indices of itineraries going to a particular arc. Indexed by arc-id.
    vector<vector<unsigned>> itinerariesToArc;
    /// @brief Precomputes demands for each particular itinerary and their relative times.
    /// The vector demandTimesForItinerary[id-itinerary] contains pairs <event_time, demand_index>.
    /// Indexed by id-itinerary.
    vector<vector<pair<unsigned, unsigned>>> demandTimesForItinerary;
    /// @brief Precomputes allotments having allotment entries with particular itinerary.
    /// Indexed by id-itinerary.
    vector<vector<unsigned>> allotmentsWithItinerary;
    /// @brief Precomputes mapping from [id-allotment][id-itinerary] to the corresponding
    /// allotment entry.
    vector<unordered_map<unsigned, unsigned>> allotmentItineraryToEntry;
    /// @brief Precomputes index of place in allotmentsWithItinerary.
    /// Indexed by [id-allotment][id-itinerary]. Helps to form allotmentContainersQ
    /// and allotmentDemandN for action and decision.
    vector<unordered_map<unsigned, unsigned>> allotmentItineraryToPlace;
    /// @brief Maps allotment id to index of select-one groups having this allotment. Recall that
    /// at most one allotment from each select-one group is accepted. The indexation is as follows:
    /// allotmentToGroups[id_allotment] -> list of groups with allotment index = id_allotment.
    vector<vector<unsigned>> allotmentToGroups;
    /// @brief Maps each travel arc id at each arrival to the first waiting arc in the same port.
    vector<unsigned> firstArcToHireAfterArc; // indexed by arc-id
    /// @brief Maps itinerary index to a relative time of corresponding arrival event.
    /// Indexed by itinerary-id. Recall that relative event time is also event index.
    vector<unsigned> arrivalTime;
    /// @brief Maps itinerary id to the duration of corresponding cut-off event.
    /// Indexed by itinerary-id.
    vector<double> itineraryIdToCutoffDuration;
    /// @brief Maps itinerary id to the real time of the corresponding cut-off event.
    /// Indexed by itinerary-id.
    vector<double> itineraryIdToRealCutoffTime;
    /// @brief Maps itinerary id to the relative time of the corresponding cut-off event.
    /// Indexed by itinerary-id.
    vector<unsigned> itineraryIdToRelativeCutoffTime;

};

/// @brief Simple notation for shared pointer to InputLinks.
using InputLinksPtr=std::shared_ptr<InputLinks>;
/// @brief Simple notation for shared pointer to constant InputLinks.
using InputLinksConstPtr=std::shared_ptr<const InputLinks>;

/**
 * @brief Creates InputLinks from InputData.
 *
 * @param[in] input InputData to use for creating InputLinks.
 * @param[out] links The pointer to a fresh InputLinks structure to fill.
 */
void createInputLinks(const InputData& input, InputLinks* links);
/**
 * @brief Prints InputLinks summary to stdout.
 *
 * @param links The InputLinks structure to summarize.
 */
void printInputLinksStats(const InputLinks& links);
/**
 * @brief Computes the amount of entries in 2-dimensional vector.
 *
 * @param target The 2-dimensional vector to process.
 * @return The computed amount of entries in target.
 */
unsigned get2dVectorSize(const vector<vector<unsigned>>& target);

} // namespace sea
