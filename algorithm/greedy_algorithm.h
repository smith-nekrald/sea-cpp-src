#pragma once

#include <vector>
#include <map>

#include "ialgorithm.h"
#include "state.h"


namespace sea {
namespace algo {

/**
 * @brief  Configures Greedy Algorithm.
 */
struct GreedyConfig {
    /// @brief Manager with InputData.
    ConstInputManagerPtr inputManager;
    /// @brief Manager with InputLinks.
    ConstLinksManagerPtr linksManager;
    /// @brief Whether algorithm should track history.
    bool trackStory;
    /// @brief Whether algorithm should try to optimize memory.
    bool memoryOptimization;
    bool ignoreSpotMarket;
    bool ignoreLongMarket;
};

struct GreedyStats {
    /// @brief Approximation on the available capacity per arc. This approximation is relevant
    /// for making greedy decisions and tracking approximate capacity utilization.
    std::vector<double> availableArcCapacity;
    /// @brief For tracking real capacity allocations at cutoff events. This tracking allows
    /// to avoid allocating more than arc capacity.
    std::vector<size_t> freeArcCapacity;
    /// @brief Stores the amount of greedily allocated per route.
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
    void allocateCapacityForItinerary(size_t idxRoute, size_t amount);

private:
    State state;
    GreedyStats greedyStats;
    bool allotmentsAsked;

    DecisionManagerPtr decisionManager;
    ConstActionManagerPtr actionManager;

    bool trackStory;
    std::map<std::string, std::vector<double>> story;

    const InputData input;
    const InputLinks links;

    bool ignoreSpotMarket;
    bool ignoreLongMarket;
    bool memoryOptimization;
};

} // namespace algo
} // namespace sea

