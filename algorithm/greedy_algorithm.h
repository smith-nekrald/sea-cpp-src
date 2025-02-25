/**
 * @file greedy_algorithm.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines Greedy Algorithm - fast baseline which makes allotment decisions on
 * the first-come-first-serve basis, and makes pricing decisions greedily on the basis
 * of local optimality.
 * @copyright (c) Smith School of Business, 2023
 */
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
    /// @brief If set to true, spot market is ignored.
    bool ignoreSpotMarket;
    /// @brief If set to true, allotment market is ignored (no allotments accepted).
    bool ignoreLongMarket;
};


/**
 * @brief Greedy algorithm. Allotments are selected on the first-come-first-served if fits,
 * and spot prices are set to optimize local revenue.
 */
class GreedyAlgorithm: public IAlgorithm {
public:
    /**
     * @brief Constructore for Greedy Algorithm.
     *
     * @param config Greedy algorithm configuration.
     */
    GreedyAlgorithm(const GreedyConfig& config);
    /**
     * @brief Builds name for the algorithm.
     *
     * @return The name of the algorithm.
     */
    virtual std::string getName() const override;
    /**
     * @brief Notifies algorithm on the environment response.
     *
     * @param action Manager with action containing environment response.
     */
    virtual void submitAction(ConstActionManagerPtr action) override;
    /**
     * @brief Request the next decision.
     *
     * @return Manager with decision.
     */
    virtual ConstDecisionManagerPtr makeDecision() override;
    /**
     * @brief Resets inner state. Essentially, resets State and GreedyStats.
     * Also resets allotmentsAsked and history.
     */
    virtual void reset() override;
    /**
     * @brief Method, required from the interface. Current implementation does nothing.
     */
    virtual void synchronizeStrategies() override;
    /**
     * @brief Returns execution history. History is only filled when trackStory is true.
     * Right now, no history is tracked regardless.
     *
     * @return Map from measurement name to a list of measurements.
     */
    virtual std::map<std::string, std::vector<double>> getStory() const override;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~GreedyAlgorithm() {};

protected:
    /**
     * @brief Makes greedy allotment decision. First-come first served.
     *
     * @return Manager with filled decision.
     */
    DecisionManagerPtr provideAllotments();
    /**
     * @brief Makes greedy spot decision. Decides on the type of event and redirects
     * to the corresponding method.
     *
     * @return Manager with filled decision.
     */
    DecisionManagerPtr makeSpotDecision();
    /**
     * @brief Processes cut-off event. Essentially, hires containers and loads all what is
     * loadable.
     *
     * @param event The cut-off event to process.
     */
    void processCutoff(const InputData::Event& event);
    /**
     * @brief Processes pricing event. Pricing is locally greedy, subject to locally optimal
     * revenue with known demand and all capacity available.
     *
     * @param event The pricing event to process.
     */
    void processPricing(const InputData::Event& event);
    /**
     * @brief Processes arrival event. Everything arrived is offhired.
     *
     * @param event The arrival event to process.
     */
    void processArrival(const InputData::Event& event);
    /**
     * @brief Processes offhiring. Everything available is offhired.
     *
     * @param event The current event to process.
     */
    void processOffhiring(const InputData::Event& event);

private:
    /**
     * @brief Verifies if the current allotment can get accepted.
     *
     * @param idxAllotment The index of allotment.
     * @return If there is enough capacity and one-group-selection fits, returns true. Otherwise
     * returns false.
     */
    bool checkIfAllotmentAvailable(size_t idxAllotment) const;
    /**
     * @brief Computes capacity available for greedy pricing on a fixed itinerary.
     *
     * @param idxItinerary The index of itinerary.
     * @return double The capacity available for greedy itinerary pricing.
     */
    double computeGreedyCapacityForItinerary(size_t idxItinerary) const;
    /**
     * @brief Computes capacity available for allocation at cut-off on a fixed itinerary.
     *
     * @param idxItinerary The index of itinerary.
     * @return size_t The available capacity for loading.
     */
    size_t computeAllocationCapacityForItinerary(size_t idxItinerary) const;
    /**
     * @brief Allocates capacity within algorithm-specific structures. Essentially, updates
     * greedyStats.
     *
     * @param idxRoute The index of itinerary.
     * @param amount The amount loaded for itinerary.
     */
    void allocateCapacityForItinerary(size_t idxRoute, size_t amount);

private:
    /// @brief State for tracking trajectory evaluation.
    State state;
    /// @brief Statistics and structures specific to greedy algorithm.
    BaselineStats greedyStats;
    /// @brief If allotment decision is already made.
    bool allotmentsAsked;

    /// @brief Manager with decision.
    DecisionManagerPtr decisionManager;
    /// @brief Manager with action.
    ConstActionManagerPtr actionManager;

    /// @brief If true, stores execution history.
    bool trackStory;
    /// @brief The place to store running history.
    std::map<std::string, std::vector<double>> story;

    /// @brief Input Data, statistics about the market.
    const InputData input;
    /// @brief Input Links, structure for easy indexation within Input Data.
    const InputLinks links;

    /// @brief If true, spot market is ignored.
    bool ignoreSpotMarket;
    /// @brief If true, allotment market is ignored.
    bool ignoreLongMarket;
    /// @brief If true, algorithm tries to apply memory optimization.
    bool memoryOptimization;
};

} // namespace algo
} // namespace sea

