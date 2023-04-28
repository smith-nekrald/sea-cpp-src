#pragma once

#include "../types.h"

#include <vector>
#include <set>
#include <memory>

#include <experimental/optional>

namespace sea {

struct ShowRate {
    double estimatedProba;
};

struct Demand {
    enum class Type {
        linear,
        exponential
    };

    Type type;

    // linear
    double additive;
    double multiplicative;

    // exponential
    double scale;
    double sensitivity;
};

struct InputData {

    template <typename T>
    using Optional = std::experimental::optional<T>;

    struct Port {
        ui32 id;
        double hiringCost;
        double offHiringCost;
        double loadCost;
        double offLoadCost;
        double storageCost;
        ui32 initialContainerCount;
        ui32 finalContainerCount;
    };

    struct Node {
        ui32 id;
        ui32 portId;
        double realTime;

        enum class Type {
            arrival,
            departure
        };

        Type type;
    };

    struct Vessel {
        ui32 id;
        double speedAvg;
        double capacity;

        std::vector<ui32> way;
    };

    struct Arc {
        ui32 id;
        ui32 fromNode;
        ui32 toNode;
        Optional<ui32> vesselId;

        enum class Type {
            reload,
            wait,
            travel
        };

        Type type;
    };

    struct Itinerary {
        ui32 id;
        double returnPrice;
        double preparedCapacity;
        double cost;
        double emptyCost;
        double declineCost;
        std::vector<ui32> orderedArcs;

        ShowRate showRate;
    };

    struct AllotmentEntry {
        ui32 id;
        ui32 itinerary;
        ui32 productAmount;
        double cancellationPrice;
        double price;

        ShowRate showRate;
    };

    struct Allotment {
        ui32 id;
        std::vector<ui32> entries;
    };

    struct Event {
        std::vector<ui32> relatedItineraryIds;
        std::vector<Demand> demands;
        double duration;
        double realTime;
        ui32 relativeTime;

        Optional<ui32> basedNode;
        Optional<ui32> basedArc;

        enum class Type {
            pricing,
            arrival,
            cutoff
        };

        Type type;
    };

    std::vector<Port> ports;
    std::vector<Vessel> vessels;
    std::vector<Node> nodes;
    std::vector<Arc> arcs;

    std::vector<AllotmentEntry> allotmentEntries;
    std::vector<Allotment> allotments;
    std::vector<std::vector<ui32> > allotmentGroups;

    std::vector<Itinerary> itineraries;
    std::vector<Event> events;
    double leaseCost;

};  // InputData

void printInputDataStats(const InputData& data);
std::string eventTypeToName(const InputData::Event::Type& type);

using InputDataPtr=std::shared_ptr<InputData>;
using InputDataConstPtr=std::shared_ptr<const InputData>;

}   // namespace sea
