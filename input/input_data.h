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
        unsigned id;
        double hiringCost;
        double offHiringCost;
        double loadCost;
        double offLoadCost;
        double storageCost;
        unsigned initialContainerCount;
        unsigned finalContainerCount;
    };

    struct Node {
        unsigned id;
        unsigned portId;
        double realTime;

        enum class Type {
            arrival,
            departure
        };

        Type type;
    };

    struct Vessel {
        unsigned id;
        double speedAvg;
        double capacity;

        std::vector<unsigned> way;
    };

    struct Arc {
        unsigned id;
        unsigned fromNode;
        unsigned toNode;
        Optional<unsigned> vesselId;

        enum class Type {
            reload,
            wait,
            travel
        };

        Type type;
    };

    struct Itinerary {
        unsigned id;
        double returnPrice;
        double preparedCapacity;
        double cost;
        double emptyCost;
        double declineCost;
        std::vector<unsigned> orderedArcs;

        ShowRate showRate;
    };

    struct AllotmentEntry {
        unsigned id;
        unsigned itinerary;
        unsigned productAmount;
        double cancellationPrice;
        double price;

        ShowRate showRate;
    };

    struct Allotment {
        unsigned id;
        std::vector<unsigned> entries;
    };

    struct Event {
        std::vector<unsigned> relatedItineraryIds;
        std::vector<Demand> demands;
        double duration;
        double realTime;
        unsigned relativeTime;

        Optional<unsigned> basedNode;
        Optional<unsigned> basedArc;

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
    std::vector<std::vector<unsigned> > allotmentGroups;

    std::vector<Itinerary> itineraries;
    std::vector<Event> events;
    double leaseCost;

};  // InputData

void printInputDataStats(const InputData& data);
std::string eventTypeToName(const InputData::Event::Type& type);

using InputDataPtr=std::shared_ptr<InputData>;
using InputDataConstPtr=std::shared_ptr<const InputData>;

}   // namespace sea
