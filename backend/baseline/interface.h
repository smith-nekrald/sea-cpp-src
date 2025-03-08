/**
 * @file interface.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines interfaces for allotment and spot sorters and metrics.
 * @copyright (c) Smith School of Business, 2025
 */
#pragma once

#include "baseline_stats.h"
#include "../../input/input_data.h"

#include <vector>
#include <string>

namespace sea {
namespace backend {

/**
 * @brief Interface for allotment sorter class. 
 * Orders allotment ids in a certain way
 * for further baseline algorithm consideration.
 */
class IAllotmentSorter {
public:
    /**
     * @brief Constructor for IAllotmentSorter.
     */
    IAllotmentSorter() {};
    /**
     * @brief Virtual method for selecting order of allotments.
     * 
     * @return Vector of allotment ids in a certain selected order.
     */
    virtual std::vector<unsigned> selectOrder() const = 0;
    /**
     * @brief Virtual method for getting the name of the allotment sorter.
     * 
     * @return Name of the allotment sorter.
     */
    virtual std::string getName() const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~IAllotmentSorter() {};
};

/**
 * @brief Interface for metric evaluating the quality of allotment order.
 * Higher metric is better, metric example - expected profit.
 */
class IAllotmentOrderMetric {
public:
    /**
     * @brief Constructor for IAllotmentOrderMetric.
     */
    IAllotmentOrderMetric() {};
    /**
     * @brief Virtual method to compute the metric value for the given allotment order.
     * 
     * @param allotmentOrder The order of allotments to evaluate.
     * @return The score (metric value) for the given allotment order.
     */
    virtual double score(const std::vector<unsigned>& allotmentOrder) const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~IAllotmentOrderMetric() {};
};

/**
 * @brief Interface for itinerary sorting at spot pricing events.
 * Orders priced itineraries in a certain way for further baseline 
 * algorithm consideration. 
 */
class ISpotSorter {
public:
    /**
     * @brief Constructor for ISpotSorter.
     */
    ISpotSorter() {};
    /**
     * @brief Virtual method for selecting order of itineraries at spot pricing events.
     * 
     * @param event The current pricing event.
     * @param stats The current state represented with BaselineStats.
     * @return Vector of relevant itinerary ids in a certain selected order.
     */
    virtual std::vector<unsigned> selectOrder(
            const InputData::Event& event, const BaselineStats& stats) const = 0;
    /**
     * @brief Virtual method for getting the name of the spot sorter.
     * 
     * @return Name of the spot sorter.
     */
    virtual std::string getName() const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ISpotSorter() {};
};

/**
 * @brief Interface for metric evaluating the quality of itinerary order at spot pricing events.
 * Higher metric is better, metric example - proxy for the expected profit.
 */
class ISpotOrderMetric {
public:
    /**
     * @brief Constructor for ISpotOrderMetric.
     */
    ISpotOrderMetric() {};
    /**
     * @brief Virtual method to compute the metric value for 
     * the given itinerary order at spot pricing events.
     * 
     * @param spotOrder The order of itineraries to evaluate.
     * @param event The current pricing event.
     * @param stats The current state represented with BaselineStats.
     * @return The score (metric value) for the given itinerary 
     *      order at the current spot pricing event.
     */
    virtual double score(const std::vector<unsigned>& spotOrder,
            const InputData::Event& event, const BaselineStats& stats) const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ISpotOrderMetric() {};
};

} // namespace backend
} // namespace sea
