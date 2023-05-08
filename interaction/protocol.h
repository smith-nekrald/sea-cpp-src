/**
 * @file protocol.h
 * 
 * @brief Speficies interaction protocol. The interaction happens through two entities, 
 * called Decision and Action. Decision represents the algorithm policy output, and Action
 * represents the reaction of environment.
 *  
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>

#include "../common.h"
#include "../input/input_data.h"

namespace sea {

/**
 * @brief Decision is a structure containing all fields relevant to all possible decisions.
 * During execution, only some of these fields are relevant. Among the decisions are pricing,
 * amounts to hire and off-hire, amounts of empty and non-empty containers to carry, etc. 
 */
struct Decision {
    /// @brief The relative time of the decision. 
    unsigned int time;

    /// @brief  Amount of containers to off-hire in ports. The indexation is [time][port].
    std::vector<std::vector<unsigned int>> offHiredInPortS;
    
    /// @brief Spot market prices. The indexation is [time][<id-itinerary, price>].
    std::vector<std::vector<std::pair<unsigned int, double>>> prices;
    
    /// @brief Amount of empty containers to take by itinerary. Indexed by id-itinerary.
    std::vector<unsigned int> emptyContainersZ; 

    /// @brief Amount of non-empty containers assigned by itinerary. Indexed by id-itinerary.
    std::vector<unsigned int> nonEmptyContainersQ;

    /// @brief Amount of non-empty containers assigned by allotment and itinerary. The indexation
    /// is via [contract-id][<id-itinerary, Q>].
    std::vector<std::vector<std::pair<unsigned int, unsigned int>>> allotmentContainersQ;

    /// @brief Amount of containers to hire. Indexed by arc id.
    std::vector<unsigned int> hiredY;

    /// @brief The decision on allotments. True if accepted, False if rejected. 
    /// Indexed by contract id.
    std::vector<bool> allotmentAccepted;

    /**
     * @brief Operator to output decision to a stream.
     * 
     * @param out[out] The stream to write the decision into. 
     * @param decision The deicison to output into the stream.
     * 
     * @return The updated stream for further processing.
     */
    friend std::ostream& operator<<(std::ostream& out, const Decision& decision);
};

/// @brief Shortcut for a smart pointer to decision.
typedef std::shared_ptr<Decision> DecisionPtr;
/// @brief Shortcut for a smart pointer to a constant decision.
typedef std::shared_ptr<const Decision> ConstDecisionPtr;

/**
 * @brief Initializes a Decision based on the input data. Essentially, sets 
 * the lengths of all relevant fields and default values.
 * 
 * @param input The input data to use for initialization. 
 * @param emptyDecision A pointer to an empty decision to initialize.
 */
void createDecision(const InputData& input, Decision* emptyDecision);

/**
 * @brief Loads a Decision from file.
 * 
 * @param pathToFile The path to the file to read the decision from.
 * @param emptyDecision A pointer to an empty decision to fill.
 */
void loadFromFile(const std::string& pathToFile, Decision* emptyDecision);

/**
 * @brief Writes Decision to file.
 * 
 * @param pathToFile The path of the file to export the decision.
 * @param filledDecision The decision to export.
 */
void writeToFile(const std::string& pathToFile, const Decision& filledDecision);


/**
 * @brief Action represents the response of the environment at each event subject to
 * the decisions made. Such data includes the amount of bookings, shown demand in spotMarket, 
 * and shown demand in the allotment market.
 */
struct Action {
    unsigned int time;
    
    /// @brief Represents the shown demand in spot market. Indexed by id-itinerary.
    std::vector<unsigned int> spotMarketDemandN; 

    /// @brief Represents the spot market bookigns as the response to the set price.
    /// Indexed by [time][id-itinerary].
    std::vector<std::vector<unsigned int>> bookingsB;  // indexed by [time][id-itinerary]

    /// @brief Represetns the shown demand in allotment market. 
    /// Indexed in the following fashion: [contract-id][<id-itinerary, N>].
    std::vector<std::vector<std::pair<unsigned int, unsigned int>>> allotmentDemandN;

    /**
     * @brief Writes Action to a stream.
     * 
     * @param out[out] The stream that receives the action.
     * @param action The Action to write into the stream.
     * 
     * @return The stream where the Action is written for a potential further use.
     */
    friend std::ostream& operator<<(std::ostream& out, const Action& action);
};

/// @brief Shortcut for the smart pointer to an Action.
typedef std::shared_ptr<Action> ActionPtr;
/// @brief Shortcut for the smart pointer to a constant Action.
typedef std::shared_ptr<const Action> ConstActionPtr;

/**
 * @brief Initializes an Action object from input data. Essentially, sets the sizes and default
 * internal values of the corresponding vectors.
 * 
 * @param input The input data to base the initialization on.
 * @param emptyAction A pointer to an empty action object to initialize.
 */
void createAction(const InputData& input, Action* emptyAction);

/**
 * @brief Loads an Action object from file.
 * 
 * @param pathToFile The path to the file the action is read from.
 * @param emptyAction A pointer to an empty Action to fill. 
 */
void loadFromFile(const std::string& pathToFile, Action* emptyAction);

/**
 * @brief Writes an Action object to file.
 * 
 * @param pathToFile The path to the file for storing the Action.
 * @param filledAction The Action to store into the file.
 */
void writeToFile(const std::string& pathToFile, const Action& filledAction);

} // namespace sea
