/**
 * @file strategic_algorithm.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares Strategic Algorithm - entity that delegates allotment decision to
 * Allotment Strategy and spot decisions to Spot Strategy.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <memory>

#include "ialgorithm.h"
#include "state.h"
#include "../strategy/general_strategy.h"
#include "../interaction/iinteractor.h"

namespace sea {
namespace algo {

/**
 * @brief Configures Strategic Algorithm. Essentially, specifies strategy for selecting allotments
 * and sport market behavior.
 */
struct StrategicAlgorithmConfig {
    /// @brief Strategy for selecting allotments.
    strategy::IAllotmentStrategyPtr allotmentStrategy;
    /// @brief Strategy for processing spot market.
    strategy::ISpotMarketStrategyPtr spotMarketStrategy;
};

/**
 * @brief Strategic algorithm is an algorithm which delegates allotment decision to
 * allotment market strategy, and spot market interation to spot market strategy.
 * Functions makeDecision and submitAction are virtual, but default implementation is provided.
 */
class StrategicAlgorithm : public IAlgorithm {
public:
    /**
     * @brief Constructor for a new Strategic Algorithm.
     *
     * @param aConfig Configuration of the algorithm. Specifies spot and allotment behavior.
     */
    explicit StrategicAlgorithm(const StrategicAlgorithmConfig& aConfig);

    /**
     * @brief Makes decision depending on the event and stage.
     *
     * @return The decision made.
     */
    virtual ConstDecisionManagerPtr makeDecision() override;
    /**
     * @brief Receives action depending on the event and state.
     *
     * @param action The action with corresponding environment information.
     */
    virtual void submitAction(ConstActionManagerPtr action) override;
    /**
     * @brief Resets inner state for further use with another trajectory.
     */
    virtual void reset() override;
    /**
     * @brief Synchronizes strategies, if needed.
     */
    virtual void synchronizeStrategies() override;
    /**
     * @brief Makes algorithm name from the name of allotment and spot strategies.
     *
     * @return Formed algorithm name.
     */
    virtual std::string getName() const override {
        return "StrategicAlgorithm:<" + config.allotmentStrategy->getName() + "," +
            config.spotMarketStrategy->getName() + ">";
    };
    /**
     * @brief Get the progress history.
     *
     * @return Map from measurement ket to the sequence of measurements.
     */
    virtual std::map<std::string, std::vector<double>> getStory() const override {
        auto story = config.spotMarketStrategy->getStory();
        if (config.spotMarketStrategy->isStoryTracked()) {
            story["allotment_value"].push_back(config.allotmentStrategy->getValueEstimation());
        }
        return story;
    }
    /**
     * @brief Virtual destructor. To ensure that C++ polymorphism works properly.
     */
    virtual ~StrategicAlgorithm() {};

private:
    /**
     * @brief Logs that startegic algorithm is created.
     */
    void logStrategicAlgorithmCreated() const;
    /**
     * @brief Logs the selected allotments.
     *
     * @param decisionManager The Decision Manager with selected allotments.
     */
    void logSelectedAllotments(ConstDecisionManagerPtr decisionManager) const;

protected:
    /// @brief Configuration. Spot Market Strategy and Allotment Strategy.
    StrategicAlgorithmConfig config;
    /// @brief If true, allotments are already requested. False otherwise.
    bool allotmentsAsked = false;
};

/// @brief Simple notation for shared pointer onto Strategic Algorithm.
typedef std::shared_ptr<StrategicAlgorithm> StrategicAlgorithmPtr;

} // namespace algo

} // namespace sea
