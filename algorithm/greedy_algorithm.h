#pragma once

#include <vector>
#include <map>

#include "ialgorithm.h"
#include "state.h"


namespace sea {
namespace algo {

struct GreedyConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
    bool trackStory;
    bool memoryOptimization;
};

struct GreedyStats {
    std::vector<double> availableArcCapacity;
    std::vector<double> allocatedSpotRouteCapacity;
    std::vector<double> allocatedLongEntryCapacity;
    std::vector<bool> ifSelectedFromGroup;
};

class GreedyAlgorithm: public IAlgorithm {
public:
    GreedyAlgorithm(const GreedyConfig& config);
    virtual std::string getName() const override;
    virtual void submitAction(ConstActionManagerPtr action) override;
    virtual ConstDecisionManagerPtr makeDecision() override;
    virtual void reset() override;
    virtual void synchronizeStrategies() override;
    virtual std::map<std::string, std::vector<double>> getStory() const override;
    virtual ~GreedyAlgorithm() {};

protected:
    DecisionManagerPtr provideAllotments();
    DecisionManagerPtr makeSpotDecision();
    void processCutoff(const InputData::Event& event);
    void processPricing(const InputData::Event& event);
    void processArrival(const InputData::Event& event);
    void processOffhiring(const InputData::Event& event);

private:
    double computeGreedyCapacityForItinerary(size_t idxItinerary) const;
    bool checkIfAllotmentAvailable(size_t idxAllotment) const;

private:
    State state;
    GreedyStats greedyStats;
    bool allotmentsAsked;
    bool memoryOptimization;

    DecisionManagerPtr decisionManager;
    ConstActionManagerPtr actionManager;

    bool trackStory;
    std::map<std::string, std::vector<double>> story;

    const InputData input;
    const InputLinks links;
};

} // namespace algo
} // namespace sea

