// API for IpoptIndexMap class methods and related IO.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
#include "index_map.h"
#include "../../common.h"

#include <limits>
#include <iostream>
#include <map>
#include <vector>

namespace sea {
namespace backend {


void initIndexMapWithMaxIndex(const InputData& input, IpoptIndexMap* indexMap) {
    const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();

    // For variables.
    indexMap->variableCount = 0;

    indexMap->timeItineraryToDemandIndex.resize(input.events.size());
    indexMap->timeItineraryToDemandIndex.shrink_to_fit();
    for (auto& subVector : indexMap->timeItineraryToDemandIndex) {
        subVector.assign(input.itineraries.size(), MAX_INDEX);
        subVector.shrink_to_fit();
    }

    indexMap->arcToHired.assign(input.arcs.size(), MAX_INDEX);
    indexMap->arcToHired.shrink_to_fit();

    indexMap->idItineraryToQIndex.assign(input.itineraries.size(), MAX_INDEX);
    indexMap->idItineraryToQIndex.shrink_to_fit();

    indexMap->idItineraryToZIndex.assign(input.itineraries.size(), MAX_INDEX);
    indexMap->idItineraryToZIndex.shrink_to_fit();

    indexMap->allotmentItineraryToQIndex.resize(input.allotments.size());
    indexMap->allotmentItineraryToQIndex.shrink_to_fit();
    for (auto& subVector : indexMap->allotmentItineraryToQIndex) {
        subVector.assign(input.itineraries.size(), MAX_INDEX);
        subVector.shrink_to_fit();
    }

    indexMap->allotmentToUIndex.assign(input.allotments.size(), MAX_INDEX);
    indexMap->allotmentToUIndex.shrink_to_fit();

    indexMap->timeToSIndex.assign(input.events.size(), MAX_INDEX);
    indexMap->timeToSIndex.shrink_to_fit();

    // For constraints.
    indexMap->constraintCount = 0;
    indexMap->arcCapacityConstraints.assign(input.arcs.size(), MAX_INDEX);
    indexMap->arcCapacityConstraints.shrink_to_fit();

    indexMap->portPositiveCutoffArcConstraints.assign(input.arcs.size(), MAX_INDEX);
    indexMap->portPositiveCutoffArcConstraints.shrink_to_fit();

    indexMap->portPositiveArrivalArcConstraints.assign(input.arcs.size(), MAX_INDEX);
    indexMap->portPositiveArrivalArcConstraints.shrink_to_fit();

    indexMap->spotQNConstraints.assign(input.itineraries.size(), MAX_INDEX);
    indexMap->spotQNConstraints.shrink_to_fit();

    indexMap->finalContainerConstraints.assign(input.ports.size(), MAX_INDEX);
    indexMap->finalContainerConstraints.shrink_to_fit();

    indexMap->groupConstraints.assign(input.allotmentGroups.size(), MAX_INDEX);
    indexMap->groupConstraints.shrink_to_fit();

    indexMap->variableToDescription.clear();
    indexMap->constraintToDescription.clear();
}

void createIndexMap(const InputData& input, IpoptIndexMap* indexMap, bool initDescriptions) {
    initIndexMapWithMaxIndex(input, indexMap);
    indexMap->constraintCount++; // need to offset all constraint indices by 1
    addIpoptIndexMap(input, indexMap, initDescriptions);
    indexMap->constraintCount--;
}

void addIpoptIndexMap(const InputData& input, IpoptIndexMap* indexMap, bool initDescriptions) {

    auto& variableToDescription = indexMap->variableToDescription;
    auto& constraintToDescription = indexMap->constraintToDescription;

    for (const auto& event : input.events) {
        unsigned relativeTime = event.relativeTime;
        // d_t
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned idItinerary : event.relatedItineraryIds) {
                if (initDescriptions) {
                    variableToDescription[indexMap->variableCount] =
                        "demand_itinerary_" + std::to_string(idItinerary) 
                        + "_at_time_" + std::to_string(relativeTime);
                }
                indexMap->timeItineraryToDemandIndex[relativeTime][idItinerary] =
                    indexMap->variableCount++;
            }
        // y_a^H ## portPositiveCutoffArcConstraints
        } else if (event.type == InputData::Event::Type::cutoff) {
            unsigned idBasedArc = event.basedArc.value();
            if (initDescriptions) {
                variableToDescription[indexMap->variableCount] =
                    "y_a^H_for_arc_" + std::to_string(idBasedArc) 
                    + "_at_time_" + std::to_string(event.relativeTime);
                constraintToDescription[indexMap->constraintCount] =
                    "port_positive_on_cutoff_at_event_" + std::to_string(event.relativeTime);
            }
            indexMap->arcToHired[idBasedArc] = indexMap->variableCount++;
            indexMap->portPositiveCutoffArcConstraints[idBasedArc] =
                indexMap->constraintCount++;
        // s_t ## portPositiveArrivalArcConstraints
        } else if (event.type == InputData::Event::Type::arrival) {
            unsigned idBasedArc = event.basedArc.value();
            if (initDescriptions) {
                variableToDescription[indexMap->variableCount] =
                    "offhire_s_t_at_time_" + std::to_string(event.relativeTime);
                constraintToDescription[indexMap->constraintCount] =
                    "port_positive_on_arrival_at_event_" + std::to_string(event.relativeTime);
            }
            indexMap->timeToSIndex[relativeTime] = indexMap->variableCount++;
            indexMap->portPositiveArrivalArcConstraints[idBasedArc] =
                indexMap->constraintCount++;
        }
    }

    // Q_r, Z_r ## spotQNConstraints
    for (const auto& itinerary : input.itineraries) {
        if (initDescriptions) {
            variableToDescription[indexMap->variableCount] = 
                "Q_r_for_itinerary_" + std::to_string(itinerary.id);
        }
        indexMap->idItineraryToQIndex[itinerary.id] = indexMap->variableCount++;
        if (initDescriptions) {
            variableToDescription[indexMap->variableCount] = 
                "Z_r_for_itinerary_" + std::to_string(itinerary.id);
            constraintToDescription[indexMap->constraintCount] = 
                "SpotQN_constraint_for_itinerary_" + std::to_string(itinerary.id);
        }
        indexMap->idItineraryToZIndex[itinerary.id] = indexMap->variableCount++;
        indexMap->spotQNConstraints[itinerary.id] = indexMap->constraintCount++;
    }

    // Q_i^r, u_i
    for (const auto& allotment : input.allotments) {
        if (initDescriptions) {
            variableToDescription[indexMap->variableCount] = 
                "u_i_for_allotment_" + std::to_string(allotment.id);
        }
        indexMap->allotmentToUIndex[allotment.id] = indexMap->variableCount++;
        for (unsigned idEntry : allotment.entries) {
            const auto& entry = input.allotmentEntries[idEntry];
            unsigned idItinerary = entry.itinerary;

            if (initDescriptions) {
                variableToDescription[indexMap->variableCount] = "Q^i_r_for_allotment_" 
                    + std::to_string(allotment.id) + "_itinerary_" + std::to_string(idItinerary);
            }
            indexMap->allotmentItineraryToQIndex[allotment.id][idItinerary] = 
                indexMap->variableCount++;
        }
    }
    // arcCapacityConstraints
    for (const auto& arc : input.arcs) {
        if (arc.type == InputData::Arc::Type::travel) {
            if (initDescriptions) {
                constraintToDescription[indexMap->constraintCount] =
                    "arcCapacityConstraint_for_arc_" + std::to_string(arc.id);
            }
            indexMap->arcCapacityConstraints[arc.id] = indexMap->constraintCount++;
        }
    }

    // finalContainerConstraints
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        if (initDescriptions) {
            constraintToDescription[indexMap->constraintCount] =
                "finalContainerConstraint_for_port_" + std::to_string(idPort);
        }

        indexMap->finalContainerConstraints[idPort] = indexMap->constraintCount++;
    }

    // groupConstraints
    for (unsigned idGroup = 0; idGroup < input.allotmentGroups.size(); ++idGroup) {
        if (initDescriptions) {
            constraintToDescription[indexMap->constraintCount] =
                "groupConstraint_for_group_"  + std::to_string(idGroup);
        }
        indexMap->groupConstraints[idGroup] = indexMap->constraintCount++;
    }
    indexMap->allotmentItineraryQConstraints.resize(input.itineraries.size());
    indexMap->allotmentItineraryQConstraints.shrink_to_fit();
    for (unsigned idItinerary = 0; idItinerary < indexMap->allotmentItineraryQConstraints.size(); 
            idItinerary++) {
        auto& allotmentQConstraints = indexMap->allotmentItineraryQConstraints[idItinerary];
        allotmentQConstraints.resize(input.allotments.size());
        allotmentQConstraints.shrink_to_fit();

        for (auto& constraintId: allotmentQConstraints) {
            constraintId = indexMap->constraintCount++;
        }
    }
}


template<typename Writer>
void printIndexMapStatsGeneral(Writer&  out, const IpoptIndexMap& indexMap) {
        out << "=======================" << "\n";
        out << "IndexMap Statistics: " << "\n";
        // Statistics related to variables.
        out << "variable count = " << indexMap.variableCount << "\n";
        std::pair<unsigned, unsigned> statStorage;

        statStorage = getUsefulIndexCount(indexMap.timeItineraryToDemandIndex);
        out << "timeItineraryToDemandIndex: field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.arcToHired);
        out << "arcToHired: field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.idItineraryToQIndex);
        out << "idItineraryToQIndex: field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.idItineraryToZIndex);
        out << "idItineraryToZIndex: field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.allotmentItineraryToQIndex);
        out << "allotmentItineraryToQIndex: field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.allotmentToUIndex);
        out << "allotmentToUIndex: field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.timeToSIndex);
        out << "timeToSIndex: field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        // Statistics related to constraints.
        out << "constraint count = " << indexMap.constraintCount << "\n";
        statStorage = getUsefulIndexCount(indexMap.arcCapacityConstraints);
        out << "arcCapacityConstraints : field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.portPositiveCutoffArcConstraints);
        out << "portPositiveCutoffArcConstraints : field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.portPositiveArrivalArcConstraints);
        out << "portPositiveArrivalArcConstraints : field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.spotQNConstraints);
        out << "spotQNConstraints : field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.finalContainerConstraints);
        out << "finalContainerConstraints: field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";

        statStorage = getUsefulIndexCount(indexMap.groupConstraints);
        out << "groupConstraints: field count = " << statStorage.second 
            << " useful fields(real constraints) = " << statStorage.first << "\n";
        out << "memory usage now = " << getMemUsage() / (1024. * 1024) << "MB" << "\n";
        out << "=======================" << "\n";
}

void printIndexMapStats(log4cpp::CategoryStream& logger, const IpoptIndexMap& indexMap) {
    printIndexMapStatsGeneral(logger, indexMap);
}

void printIndexMapStats(std::ostream& out, const IpoptIndexMap& indexMap) {
    printIndexMapStatsGeneral(out, indexMap);
}

} // namespace backend

void writeToFile(
        const std::string& filePath,
        const backend::IpoptIndexMap& indexMap) {
    std::ofstream writer(filePath, std::ios::binary);
    ::cereal::BinaryOutputArchive oa(writer);
    oa << indexMap;
}

void loadFromFile(
        const std::string& filePath,
        backend::IpoptIndexMap* indexMap) {
    std::ifstream reader(filePath, std::ios::binary);
    ::cereal::BinaryInputArchive ia(reader);
    ia >> *indexMap;
}

} // namespace sea
