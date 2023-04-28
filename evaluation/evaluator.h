#pragma once

#include "statistics.h"
#include "../algorithm/ialgorithm.h"
#include "../manager.h"
#include "../common.h"
#include "../types.h"

namespace sea {

struct EvaluatorConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
    std::experimental::optional<bool> needMemory; // = false;
    std::experimental::optional<bool> keepStory;
};

class Evaluator {

public:
    explicit Evaluator(const EvaluatorConfig& config);
    Statistics calc(algo::IAlgorithmPtr algo, ConstMarketManagerPtr market);
    std::map<std::string, std::vector<double>> getAlgoStory() const {
        return algoStory;
    }
    std::map<std::string, std::vector<double>> getEvalStory() const {
        return evalStory;
    }

private:
    void processAllotmentDecision();
    void processCutoffDecision(const InputData::Event& event);
    void processArrival(const InputData::Event& event);

    void updateContainersInPorts(double durationToNextPeriod);
    void checkFinalContainersInPorts();

    void makePricingAction(const InputData::Event& event);
    void makeCutoffAction(const InputData::Event& event);

    void initStartState();

private:
    EvaluatorConfig config;
    ConstMarketManagerPtr marketManager;

    vector<bool> selectedAllotments;

    // [arc-id] -> itineraries end in this arc
    std::map<ui32, std::vector<ui32>> itinerariesEndInArc;
    // <port-id, arc-id>
    std::map<ui32, ui32> arcEndsInPort;

    // indexed by port-id
    std::vector<ui32> containersInPorts;
    // indexed by itinerary-id
    std::vector<ui32> containersAssignedOnItineraries;
    // indexed by itinerary-id
    std::vector<ui32> totalBookings;
    // indexed by arc-id
    std::vector<ui32> usedCapacity;

    ActionManagerPtr actionManager;
    ConstDecisionManagerPtr decisionManager;

    Statistics statistics;

    std::map<std::string, std::vector<double>> algoStory;
    std::map<std::string, std::vector<double>> evalStory;
};

}   // namespace sea
