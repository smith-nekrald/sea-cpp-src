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
    virtual std::vector<unsigned> selectOrder(
            const InputData::Event& event, const BaselineStats& stats) const;
    virtual std::string getName() const;
    virtual ~CompositeSpotSorter() {};

private:
    void logScorePerOrder(
            const std::string& sorterName,
            const std::vector<unsigned>& spotOrder,
            bool reverseFlag,
            double score) const;

private:
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
    std::unique_ptr<ISpotOrderMetric> metric;
    std::vector<std::unique_ptr<ISpotSorter>> sorters;
};

class SpotEventProfitMetric : public ISpotOrderMetric {
public:
    SpotEventProfitMetric(const BaselineSpotConfig& config);
    virtual double score(
            const std::vector<unsigned>& spotOrder,
            const InputData::Event& event,
            const BaselineStats& stats) const override final;
    virtual ~SpotEventProfitMetric() {};

private:
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};

} // namespace spot
} // namespace backend
} // namespace sea
