#pragma once

#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>

#include "../common.h"
#include "../input/input_data.h"

namespace sea {

struct Decision {
    ui32 time;
    // indexed by [time][port]
    std::vector<std::vector<ui32>> offHiredInPortS;
    // [time][<id-itinerary, price>]
    std::vector<std::vector<std::pair<ui32, double>>> prices;

    std::vector<ui32> emptyContainersZ;       // indexed by id-itinerary
    std::vector<ui32> nonEmptyContainersQ;    // indexed by id-itinerary

    // [contract-id][<id-itinerary, Q>]
    std::vector<std::vector<std::pair<ui32, ui32>>> allotmentContainersQ;

    std::vector<ui32> hiredY;    // indexed by arc-id

    std::vector<bool> allotmentAccepted;    // indexed by contract-id

    friend std::ostream& operator<<(std::ostream& out, const Decision& decision);
};

typedef std::shared_ptr<Decision> DecisionPtr;
typedef std::shared_ptr<const Decision> ConstDecisionPtr;

void createDecision(const InputData& input, Decision* emptyDecision);
void loadFromFile(const std::string& pathToFile, Decision* emptyDecision);
void writeToFile(const std::string& pathToFile, const Decision& filledDecision);

struct Action {
    ui32 time;
    std::vector<ui32> spotMarketDemandN;  // indexed by id-itinerary
    std::vector<std::vector<ui32>> bookingsB;  // indexed by [time][id-itinerary]

    // [contract-id][<id-itinerary, N>]
    std::vector<std::vector<std::pair<ui32, ui32>>> allotmentDemandN;

    friend std::ostream& operator<<(std::ostream& out, const Action& action);
};

typedef std::shared_ptr<Action> ActionPtr;
typedef std::shared_ptr<const Action> ConstActionPtr;

void createAction(const InputData& input, Action* emptyAction);
void loadFromFile(const std::string& pathToFile, Action* emptyAction);
void writeToFile(const std::string& pathToFile, const Action& filledAction);

} // namespace sea
