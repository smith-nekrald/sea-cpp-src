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
    // For making decisions and tracking approximate capacity utilization.
    std::vector<double> availableArcCapacity;
    // For tracking real capacity allocations. Capacity here is about real allocations at cutoff.
    std::vector<size_t> freeArcCapacity;
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
    bool checkIfAllotmentAvailable(size_t idxAllotment) const;
    double computeGreedyCapacityForItinerary(size_t idxItinerary) const;
    size_t computeAllocationCapacityForItinerary(size_t idxItinerary) const;
    size_t allocateCapacityForItinerary(size_t idxRoute, size_t amount);

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

