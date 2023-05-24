/**
 * @file input_data.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares InputData - the structure with statistical input for policy design.
 * Also declares related sub-structures and defines related API.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../types.h"

#include <vector>
#include <set>
#include <memory>

#include <experimental/optional>

namespace sea {

/**
 * @brief Show Rate represents the probability for each client to show if booked.
 */
struct ShowRate {
    /// @brief The probability that each client shows if booked.
    double estimatedProba;
};

/**
 * @brief Describes demand.
 */
struct Demand {
    /**
     * @brief Type of demand. E.g. linear, exponential, etc.
     */
    enum class Type {
        linear,     ///< Linear demand, demand = [additive + multipicative * price]^{+}
        exponential ///< Exponential demand, demand = scale * exp(-sensitivity * price)
    };

    /// @brief Specification for the demand type.
    Type type;

    // Parameters for Linear demand.

    /// @brief Additive component in Linear demand.
    double additive;
    /// @brief Multiplicative component in Linear demand.
    double multiplicative;

    // Parameters for exponential demand.

    /// @brief Scale for Exponential Demand.
    double scale;
    /// @brief Sensitivity for Exponential Demand.
    double sensitivity;
};

/**
 * @brief InputData represents all statistical knowledge that is used for designing
 * optimization policy. InputData is the data known for the algorithm and represents
 * assumptions about the environment.
 */
struct InputData {

    /// @brief Simplified notation for experimental::optional.
    /// @tparam Type Template parameter for experimental::optional.
    template <typename Type>
    using Optional = std::experimental::optional<Type>;

    /**
     * @brief Describes ports and coresponding information.
     */
    struct Port {
        /// @brief Identifier of port. Also serves as index.
        unsigned id;
        /// @brief Cost to hire a TEU in the port.
        double hiringCost;
        /// @brief Cost to offhire a TEU in the port.
        double offHiringCost;
        /// @brief Cost to load a TEU to a vessel in the port.
        double loadCost;
        /// @brief Cost to off-load a TEU from vessel in the port.
        double offLoadCost;
        /// @brief Storage cost per TEU per unit of time in the port.
        double storageCost;
        /// @brief The initial amount of TEU stored in port at the start of the season.
        unsigned initialContainerCount;
        /// @brief The final amount of TEU to remain in the port after season is over.
        unsigned finalContainerCount;
    };

    /**
     * @brief Describes nodes and corresponding information.
     * Essentially, a node is a port-time pair, a point in space-temporal space.
     */
    struct Node {
        /// @brief Describes node type. E.g. arrival node, or departure node.
        enum class Type {
            arrival,  ///< Node where some vessel arrival happens.
            departure ///< Node where some vessel departure happens.
        };
        /// @brief Identifier of the node. Also serves as index.
        unsigned id;
        /// @brief Identifier of the coresponding port.
        unsigned portId;
        /// @brief The time of the node (horizon starts at zero).
        double realTime;
        /// @brief Type of the node. Arrival or departure.
        Type type;
    };

    /**
     * @brief Describes vessels and related information.
     * Briefly, velocity, capacity and trajectory. 
     */
    struct Vessel {
        /// @brief Indentifier of the vessel.Also serves as index.
        unsigned id;
        /// @brief Average vessel velocity.
        double speedAvg;
        /// @brief Average vessel capacity.
        double capacity;
        /// @brief Vessel trajectory. The sequence of port indices vessel ships through.
        std::vector<unsigned> way;
    };

    /**
     * @brief Describes arcs and related information. Arcs are represented by two nodes,
     * the node of origin and the node of destination.
     */
    struct Arc {
        /**
         * @brief Enumerates arc types. 
         */
        enum class Type {
            reload, ///< Reload arc represents moving from one vessel to another.
            wait,   ///< Wait arc represents waiting in port.
            travel  ///< Travel arc represents transporting on a particular vessel.
        };
        /// @brief Arc identifier. Also serves as arc index.
        unsigned id;        
        /// @brief The origin node index.
        unsigned fromNode;
        /// @brief The destination node index.
        unsigned toNode;
        /// @brief The vessel identifier.
        Optional<unsigned> vesselId;
        /// @brief The type of arc. E.g. reload, wait or travel.
        Type type;
    };

    /**
     * @brief Describes itinerary and related information.
     */
    struct Itinerary {
        /// @brief Identifier of the itinerary. Also serves as index.
        unsigned id;
        /// @brief The return price. Paid if a client no-shows.
        double returnPrice;
        /// @brief Capacity allocated for the itinerary. This is an approximate value,
        /// the real capacity utilization may get higher, but is bounded by the capacity of 
        /// vessels corresponding to travel arcs. 
        double preparedCapacity;
        /// @brief The cost of transporting one non-empty TEU.
        double cost;
        /// @brief The cost of transporting one empty TEU.
        double emptyCost;
        /// @brief The cost for declining one TEU.
        double declineCost;
        /// @brief The arcs this itinerary trasports through.
        std::vector<unsigned> orderedArcs;
        /// @brief Probability information that each sport market booking shows.
        ShowRate showRate;
    };

    /**
     * @brief Describes allotment entry. Allotment entry is a part of allotment,
     * long-term arrangement for a particular itinerary. 
     */
    struct AllotmentEntry {
        /// @brief Identifier of allotment entry. Also serves as index.
        unsigned id;
        /// @brief Identifier of corresponding itinerary. 
        unsigned itinerary;
        /// @brief Amount of product to transport.
        unsigned productAmount;
        /// @brief Price per TEU, paid if arrangement is cancelled by a customer.
        double cancellationPrice;
        /// @brief Price per TEU.
        double price;
        /// @brief Probability that each TEU shows.
        ShowRate showRate;
    };

    /**
     * @brief Allotment is a set of allotment entries.
     */
    struct Allotment {
        /// @brief Identifier of allotment. Also serves as index.
        unsigned id;
        /// @brief The vector with indices of allotment entries.
        std::vector<unsigned> entries;
    };

    /**
     * @brief Represents event. E.g. pricing, arrival or cutoff.
     */
    struct Event {
        /**
         * @brief Enumerates event types.
         */
        enum class Type {
            pricing, ///< At pricing, spot market prices are announced, and demand is collected.
            arrival, ///< At arrival, some itinerary arrives to some port.
            cutoff   ///< At cutoff, itinerary starts and containers start loading to the vessel.
        };
        /// @brief The vector with related itinerary ids, when relevant.
        std::vector<unsigned> relatedItineraryIds;
        /// @brief The vector with related demands, when relevant.
        std::vector<Demand> demands;
        /// @brief Event duration, when relevant.
        double duration;
        /// @brief Real time of the event.
        double realTime;
        /// @brief Relative time. Also serves as index of the event.
        unsigned relativeTime;
        /// @brief Based node of the event, when relevant.
        Optional<unsigned> basedNode;
        /// @brief Based arc of the event, when relevant.
        Optional<unsigned> basedArc;
        /// @brief Type of the event. E.g. pricing, arrival or cutoff.
        Type type;
    };

    /// @brief Vector with all ports.
    std::vector<Port> ports;
    /// @brief Vector with all vessels.
    std::vector<Vessel> vessels;
    /// @brief Vector with all nodes.
    std::vector<Node> nodes;
    /// @brief Vector with all arcs.
    std::vector<Arc> arcs;
    /// @brief Vector with all allotment entries.
    std::vector<AllotmentEntry> allotmentEntries;
    /// @brief Vector with all allotments.
    std::vector<Allotment> allotments;
    /// @brief Vector with allotment groups. At most one allotment from each group is selected.
    std::vector<std::vector<unsigned> > allotmentGroups;
    /// @brief Vector with all itineraries.
    std::vector<Itinerary> itineraries;
    /// @brief Vector with all events.
    std::vector<Event> events;
    /// @brief Lease cost per TEU.
    double leaseCost;
};  // InputData

/**
 * @brief Prints InputData fields.
 * 
 * @param data The data to print.
 */
void printInputDataStats(const InputData& data);
/**
 * @brief Converts event type to human-readable string.
 * 
 * @param type The type of event.
 * @return Human-readable description of the event type.
 */
std::string eventTypeToName(const InputData::Event::Type& type);

/// @brief Typedef for shared pointer to an InputData.
using InputDataPtr=std::shared_ptr<InputData>;
/// @brief Typedef for a shared pointer to a constant InputData.
using InputDataConstPtr=std::shared_ptr<const InputData>;

}   // namespace sea
