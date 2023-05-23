/**
 * @file evaluator.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines Evaluator - the entity for evaluating a particular algorithm on a particular
 * market trajectory realization.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "statistics.h"
#include "../algorithm/ialgorithm.h"
#include "../manager.h"
#include "../common.h"
#include "../types.h"

namespace sea {

/**
 * @brief Configures Evaluator.
 */
struct EvaluatorConfig {
    /// @brief Manager with InputData structure.
    ConstInputManagerPtr inputManager;
    /// @brief Manager with InputLinks structure.
    ConstLinksManagerPtr linksManager;
    /// @brief If True, memory management tricks are needed.
    std::experimental::optional<bool> needMemory; 
    /// @brief If True, Evaluator is configured to keep evaluation history.
    std::experimental::optional<bool> keepStory;
};

/**
 * @brief Entity to evaluate a particular algorithm on a particular market trajectory realization.
 */
class Evaluator {

public:
    /**
     * @brief Constructor for a new Evaluator.
     * 
     * @param config Evaluator configuration.
     */
    explicit Evaluator(const EvaluatorConfig& config);
    /**
     * @brief Calculates profit obtained by algorithm algo on a trajectory market.
     * 
     * @param algo The algorithm to calculate.
     * @param market The market realization trajectory to process.
     * @return Evaluation statistics.
     */
    Statistics calc(algo::IAlgorithmPtr algo, ConstMarketManagerPtr market);
    /**
     * @brief Getter for algorithm story - the history of measurements filled by the algorithm. 
     * This history is sometimes partially filled by evaluator, e.g. with current events.
     * 
     * @return The algorithm information history.
     */
    std::map<std::string, std::vector<double>> getAlgoStory() const {
        return algoStory;
    }
    /**
     * @brief Getter for the evaluation history - the history of measurements filled by evaluator.
     * 
     * @return The evaluation history.
     */
    std::map<std::string, std::vector<double>> getEvalStory() const {
        return evalStory;
    }

private:
    /**
     * @brief Processes allotment decision (after the algorithm makes that decision).
     */
    void processAllotmentDecision();
    /**
     * @brief Processes cutoff decision (after the algorithm makes that decision).
     * 
     * @param event The corresponding current cutoff event.
     */
    void processCutoffDecision(const InputData::Event& event);
    /**
     * @brief Processes arrival event.
     * 
     * @param event The corresponding current arrival event.
     */
    void processArrival(const InputData::Event& event);
    /**
     * @brief Updates amount of containers in ports, which changes due to hiring or off-hiring.
     * Accounts for corresponding hiring/offhiring/storage costs in ports.
     * 
     * @param durationToNextPeriod The duration to store before the next event is considered.
     */
    void updateContainersInPorts(double durationToNextPeriod);
    /**
     * @brief After the final event, accounts for all changes to make the final container
     * counts equal to the desired final container counts, and applies all corresponding costs.
     */
    void checkFinalContainersInPorts();

    /**
     * @brief Computes pricing action when pricing event happens.
     * Pricing action is a response in demand for the pricing decision, etc. 
     * 
     * @param event The corresponding pricing event.
     */
    void makePricingAction(const InputData::Event& event);
    /**
     * @brief Computes cutoff action, when cutoff event happens. This action informs
     * on show counts and configuration.
     * 
     * @param event The corresponding cutoff event.
     */
    void makeCutoffAction(const InputData::Event& event);

    /**
     * @brief Initializes starting state, called before the actual evaluation happens.
     * Clears history and inner structures.
     */
    void initStartState();

private:
    /**
     * @brief Logs that evaluator started processing event.
     * 
     * @param event The event that gets processed.
     */
    void logEvaluatorLaunched(const InputData::Event& event) const;
    /**
     * @brief Logs allotment decision.
     */
    void logAllotmentDecision() const;
    /**
     * @brief Logs profits (spot profit, allotment profit, full profit). Called after
     * the evaluation is finished. 
     */
    void logProfits() const;
    /**
     * @brief Logs used capacity. Called after each event.
     */
    void logUsedCapacity() const;
    /**
     * @brief Logs spot profit.
     * 
     * @param event The current event.
     */
    void logSpotProfitAfterLastEvent(const InputData::Event& event) const;
    /**
     * @brief Logs that the evaluator is now considering pricing event.
     */
    void logAboutPricing() const;
    /**
     * @brief Logs that the evaluator is now considering arrival event.
     */
    void logAboutArrival() const;
    /**
     * @brief Logs that the evaluator is now considering offhiring event.
     */
    void logAboutOffhiring() const;
    /**
     * @brief Logs that the evaluator is now considering cutoff event.
     */
    void logAboutCutoff() const;

    /**
     * @brief Logs that evaluator has started making pricing action.
     * 
     * @param event The corresponding pricing event.
     */
    void logAboutMakingPricingActionForEvent(const InputData::Event& event) const;
    /**
     * @brief Logs bookings and revenue when processing pricing.
     * 
     * @param itinerary The corresponding itinerary.
     * @param price The set price.
     * @param bookings The collected bookings.
     */
    void logPricingBookingsRevenue(unsigned itinerary, double price, unsigned bookings) const;

    /**
     * @brief Logs that evaluator have entered makeCutoffAction method, and some stats.
     * 
     * @param event The corresponding cutoff event.
     */
    void logStartMakeCutoffAction(const InputData::Event& event) const;
    /**
     * @brief Logs per-itinerary information while executing makeCutoffAction.
     * 
     * @param itineraryId The itinerary index.
     */
    void logItineraryInfoInMakeCutoffAction(unsigned itineraryId) const;
    /**
     * @brief Logs per-allotment information while executing makeCutoffAction.
     * 
     * @param showAmountN The amount shown per corresponding allotment entry.
     * @param productAmount The amount booked per corresponding allotment entry.
     */
    void logAllotmentInfoInMakeCutoffAction(unsigned showAmountN, unsigned productAmount) const;
    /**
     * @brief Logs that evaluator has finished makeCutoffAction execution, and some stats.
     */
    void logFinishedMakeCutoffAction() const;

    /**
     * @brief Logs that evaluator has started executing processCutoffDecision, and some stats.
     * 
     * @param event The corresponding cutoff event.
     * @param hiredAmount The amount TEU hired in the corresponding port.
     * @param paidForHiring The amount paid for hiring.
     */
    void logStartProcessCutoffDecision(const InputData::Event& event, unsigned hiredAmount,
            double paidForHiring) const;
    /**
     * @brief Logs spot-itinerary information during executing processCutoffDecision.
     * 
     * @param itineraryId The considered itinerary index.
     * @param takenContainers Amount of taken containers.
     * @param emptyContainers Amount of emtpy containers.
     * @param diffDemandTaken Difference between shown and taken.
     * @param paidDeclineCost The decline cost paid.
     * @param nonEmptyTransferCost The cost paid for transporting non-empty containers.
     * @param emptyTransferCost The cost paid for transporting empty containers.
     * @param waitingCost The cost paid by containers waiting in port.
     */
    void logItinerarySpotInProcessCutoffDecision(unsigned itineraryId, unsigned takenContainers,
            unsigned emptyContainers, double diffDemandTaken, double paidDeclineCost,
            double nonEmptyTransferCost, double emptyTransferCost,  double waitingCost) const;
    /**
     * @brief Logs that evaluator has finished executing processCutoffDecision, and some stats.
     */
    void logFinishedProcessCutoffDecision() const;

private:
    /// @brief Evaluator configuration.
    EvaluatorConfig config;
    /// @brief Manager for the current market realization.
    ConstMarketManagerPtr marketManager;

    /// @brief The set of selected allotments.
    vector<bool> selectedAllotments;

    /// @brief Itineraries that end in particular arc. 
    /// Indexation:[arc-id] -> vector with ids of itineraries that end in this arc.
    std::map<unsigned, std::vector<unsigned>> itinerariesEndInArc;

    /// @brief Amount of containers in ports. Indexed by port-id.
    std::vector<unsigned> containersInPorts;
    /// @brief Amount of containers assigned to itineraries. Indexed by itinerary-id.
    std::vector<unsigned> containersAssignedOnItineraries;
    /// @brief Amount of bookings per itinerary. Indexed by itinerary-id.
    std::vector<unsigned> totalBookings;
    /// @brief Capacity utilization per arc. Indexed by arc-id.
    std::vector<unsigned> usedCapacity;

    /// @brief Manager for Action object. Evaluator modifies action.
    ActionManagerPtr actionManager;
    /// @brief Manager for Decision object. Decision object is updated by algorithm, not evaluator.
    ConstDecisionManagerPtr decisionManager;

    /// @brief Statistics contains the summary of evaluation.
    Statistics statistics;

    /// @brief Contains algorithm-provided history, if such history is collected.
    std::map<std::string, std::vector<double>> algoStory;
    /// @brief Contains evaluator-provided history, if such history is collected.
    std::map<std::string, std::vector<double>> evalStory;
};

}   // namespace sea
