/**
 * @file spot.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines spot pricing itinerary sorters and itinerary order metrics.
 * @copyright (c) Smith School of Business, 2025
 */
#pragma once

#include "interface.h"
#include "../../manager.h"

#include <string>
#include <map>

namespace sea {
namespace backend {
namespace spot {

/**
 * @brief Configuration for baseline spot itinierary
 * sorters and itinerary order metrics.
 */
struct BaselineSpotConfig {
public:
    /// @brief Input manager.
    ConstInputManagerPtr inputManager;
    /// @brief Links manager.
    ConstLinksManagerPtr linksManager;
};


/**
 * @brief Abstract spot pricing itinerary sorter. Sorts itineraries
 * based on values computed with getRouteMetric.
 */
class AbstractSpotSorter : public ISpotSorter {
public:
    /**
     * @brief Constructor for the abstract spot pricing itinerary sorter.
     *
     * @param config Baseline spot configuration.
     * @param sorterName The name of the spot sorter.
     */
    AbstractSpotSorter(const BaselineSpotConfig& config, const std::string& sorterName);
    /**
     * @brief Selects the order of itineraries for the given event.
     * Baseline algorithm is expected to further consider itineraries
     * in the provided order.
     *
     * @param event The considered pricing event.
     * @param stats Baseline stats representing the current state information.
     * @return The order of itinerary indices for the given event.
     */
    virtual std::vector<unsigned> selectOrder(
            const InputData::Event& event, const BaselineStats& stats) const override;
    /**
     * @brief Getter for the spot pricing itinerary sorter name.
     *
     * @return The spot pricing itinerary sorter name.
     */
    virtual std::string getName() const override final;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~AbstractSpotSorter() {};

protected:
    /**
     * @brief Virtual method to compute score value for a particular itinerary.
     *
     * @param stats Baseline stats representing the current state information.
     * @param demand The demand for the considered itinerary at considered pricing event.
     * @param idxItinerary The index of the considered itinerary.
     * @return The score value for a particular itinerary.
     */
    virtual double getRouteMetric(
            const BaselineStats& stats, Demand& demand, unsigned idxItinerary) const = 0;

private:
    /**
     * @brief Logs metric values for each itinerary.
     *
     * @param event The considered pricing event.
     * @param metricValues Vector with metric values for each itinerary.
     */
    void logMetricValues(
            const InputData::Event& event,
            const std::map<unsigned, double>& metricValues) const;

private:
    /// @brief Sorter name.
    const std::string name;

protected:
    /// @brief Input manager.
    ConstInputManagerPtr inputManager;
    /// @brief Links manager.
    ConstLinksManagerPtr linksManager;
};

/**
 * @brief Sorts pricing itineraries by expected total profit.
 */
class ByExpectedTotalProfit : public AbstractSpotSorter {
public:
    /**
     * @brief Constructor for ByExpectedTotalProfit spot pricing itinerary sorter.
     *
     * @param config Baseline spot configuration.
     */
    ByExpectedTotalProfit(const BaselineSpotConfig& config);
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ByExpectedTotalProfit() {};

private:
    /**
     * @brief Scores the itinerary by local expected total profit.
     *
     * @param stats Baseline stats representing the current state information.
     * @param demand The demand for the considered itinerary at considered pricing event.
     * @param idxItinerary The index of the considered itinerary.
     * @return The score value for a particular itinerary, proxy for local expected total profit.
     */
    virtual double getRouteMetric(
            const BaselineStats& stats,
            Demand& demand,
            unsigned idxItinerary) const override final;

};


/**
 * @brief Sorts pricing itineraries by expected
 * unit profit estimated before pricing.
 */
class ByExpectedUnitProfit : public AbstractSpotSorter {
public:
    /**
     * @brief Constructor for ByExpectedUnitProfit spot pricing itinerary sorter.
     *
     * @param config Baseline spot configuration.
     */

    ByExpectedUnitProfit(const BaselineSpotConfig& config);
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ByExpectedUnitProfit() {};

private:
    /**
     * @brief Scores the itinerary by local expected unit profit estimated before pricing.
     *
     * @param stats Baseline stats representing the current state information.
     * @param demand The demand for the considered itinerary at considered pricing event.
     * @param idxItinerary The index of the considered itinerary.
     * @return The score value for a particular itinerary, proxy for local expected unit profit.
     */
    virtual double getRouteMetric(
            const BaselineStats& stats,
            Demand& demand,
            unsigned idxItinerary) const override final;
};


/**
 * @brief Trivial spot pricing itinerary sorter.
 * Returns sorting in the original order.
 */
class TrivialItineraryOrder : public ISpotSorter {
public:
    /**
     * @brief Constructor for the trivial spot pricing itinerary sorter.
     */
    TrivialItineraryOrder();
    /**
     * @brief Selects the order of itineraries for the given event
     * by returning the original itinerary order.
     *
     * @param event The considered pricing event.
     * @param stats Baseline stats representing the current state information.
     * @return The order of itinerary indices for the given event, in the original order.
     */
    virtual std::vector<unsigned> selectOrder(
            const InputData::Event& event, const BaselineStats& stats) const override final;
    /**
     * @brief Getter for the trivial spot pricing itinerary sorter name.
     *
     * @return The name of the trivial itinerary sorter.
     */
    virtual std::string getName() const override final;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~TrivialItineraryOrder() {};
};

/**
 * @brief Composite spot pricing itinerary sorter.
 * Combines multiple spot pricing itinerary sorters.
 * For each sorter applied, computes order metric
 * and selects the order with the highest metric score.
 */
class CompositeSpotSorter : public ISpotSorter {
public:
    /**
     * @brief Constructor for composite spot pricing itinerary sorter.
     *
     * @param config The baseline spot configuration.
     */
    CompositeSpotSorter(const BaselineSpotConfig& config);
    /**
     * @brief Selects the order of itineraries by applying multiple sorters
     * and selecting the order with the highest metric score.
     *
     * @param event The considered pricing event.
     * @param stats Current state information.
     * @return The suggested order of itinerary indices.
     */
    virtual std::vector<unsigned> selectOrder(
            const InputData::Event& event, const BaselineStats& stats) const;
    /**
     * @brief Getter for the composite spot pricing itinerary sorter name.
     *
     * @return The name of the composite spot pricing itinerary sorter.
     */
    virtual std::string getName() const;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~CompositeSpotSorter() {};

private:
    /**
     * @brief Logs metric values for ordered vector of itineraries.
     *
     * @param sorterName The name of the sorter providing the order.
     * @param spotOrder The order of itineraries.
     * @param reverseFlag Flag indicating if the order is reversed.
     * @param score The metric score value for the order.
     */
    void logScorePerOrder(
            const std::string& sorterName,
            const std::vector<unsigned>& spotOrder,
            bool reverseFlag,
            double score) const;

private:
    /// @brief Input manager.
    ConstInputManagerPtr inputManager;
    /// @brief Links manager.
    ConstLinksManagerPtr linksManager;
    /// @brief Spot pricing itinerary order metric.
    std::unique_ptr<ISpotOrderMetric> metric;
    /// @brief Vector of spot pricing itinerary sorters.
    std::vector<std::unique_ptr<ISpotSorter>> sorters;
};

/**
 * @brief Metric for ordered vector of itineraries at spot pricing event.
 * Proxy of the total expected profit for the event.
 */
class SpotEventProfitMetric : public ISpotOrderMetric {
public:
    /**
     * @brief Constructor for SpotEventProfitMetric.
     *
     * @param config The baseline spot configuration.
     */
    SpotEventProfitMetric(const BaselineSpotConfig& config);
    /**
     * @brief Computes the expected profit obtained on first-come first-served basis
     * when itineraries are considered in the order defined at vector spotOrder and
     * priced in the locally optimal fashion.
     *
     * @param spotOrder The order of itineraries.
     * @param event The considered pricing event.
     * @param stats Current state information.
     * @return The score value for the ordered vector of itineraries.
     */
    virtual double score(
            const std::vector<unsigned>& spotOrder,
            const InputData::Event& event,
            const BaselineStats& stats) const override final;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~SpotEventProfitMetric() {};

private:
    /// @brief Input manager.
    ConstInputManagerPtr inputManager;
    /// @brief Links manager.
    ConstLinksManagerPtr linksManager;
};

} // namespace spot
} // namespace backend
} // namespace sea
