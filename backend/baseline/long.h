/**
 * @file long.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines the baseline allotment sorters and allotment order metrics.
 * @copyright (c) Smith School of Business, 2025
 */
#pragma once

#include "interface.h"
#include "../../manager.h"

#include <memory>

namespace sea {
namespace backend {
namespace allotment {


/**
 * @brief Config for baseline allotment
 * sorters and allotment order metrics.
 */
struct BaselineAllotmentConfig {
public:
    /// @brief Manager for Input Data. Configuration and
    /// statistical information.
    ConstInputManagerPtr inputManager;
    /// @brief Manager for Input Links. Precomputed indices
    /// and structures on top of input data.
    ConstLinksManagerPtr linksManager;
};


/**
 * @brief Abstract class for sorting allotments. Sorts allotments in the increasing order
 * of the metric proxy computed with virtual method getAllotmentMetric.
 */
class AbstractAllotmentSorter: public IAllotmentSorter {
public:
    /**
     * @brief Constructor for AbstractAllotmentSorter.
     *
     * @param config Configuration with input and links managers.
     * @param sorterName The name of the sorter.
     */
    AbstractAllotmentSorter(const BaselineAllotmentConfig& config, const std::string& sorterName);
    /**
     * @brief Orders allotments based on virtual method getAllotmentMetric providing metric proxy.
     *
     * @return Vector with ordered allotment identifiers (indices).
     */
    virtual std::vector<unsigned> selectOrder() const override;
    /**
     * @brief Getter for the name of the sorter.
     *
     * @return The name of the sorter.
     */
    virtual std::string getName() const override final;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~AbstractAllotmentSorter() {};

protected:
    /**
     * @brief Computes the metric proxy for the allotment.
     * Examples - expected profit, or consumed capacity.
     *
     * @param allotmentId Identifier of the allotment.
     * @return The metric proxy for the allotment.
     */
    virtual double getAllotmentMetric(unsigned allotmentId) const = 0;

private:
    /**
     * @brief Logs the metric values for the allotments.
     *
     * @param metricValues The values of the metric for the allotments.
     */
    void logMetricValues(const std::vector<double>& metricValues) const;

private:
    /// @brief The name of the sorter.
    std::string name;

protected:
    /// @brief Manager for Input Data.
    ConstInputManagerPtr inputManager;
    /// @brief Manager for Input Links.
    ConstLinksManagerPtr linksManager;
};


/**
 * @brief Random Allotment Sorter. Selects the order of allotments randomly.
 */
class RandomAllotmentSorter: public IAllotmentSorter {
public:
    /**
     * @brief Constructor for RandomAllotmentSorter.
     *
     * @param randomSeed Random seed.
     * @param nAllotments Number of allotments.
     */
    RandomAllotmentSorter(size_t randomSeed, size_t nAllotments);
    /**
     * @brief Selects the order of allotments randomly.
     *
     * @return Vector with randomly selected order of allotments.
     */
    virtual std::vector<unsigned> selectOrder() const override final;
    /**
     * @brief Getter for the name of the random allotment sorter.
     *
     * @return The name of the random allotment sorter.
     */
    virtual std::string getName() const override final;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~RandomAllotmentSorter() {};

private:
    /// @brief Random seed.
    size_t seed;
    /// @brief Number of allotments.
    size_t allotmentCount;
};


/**
 * @brief Trivial Sorter. Selects the order 1, ..., nAllotments.
 * Usually this is the original order of allotments.
 */
class TrivialSorter: public IAllotmentSorter {
public:
    /**
     * @brief Constructor for TrivialSorter.
     *
     * @param config Configuration with input and links managers.
     */
    TrivialSorter(const BaselineAllotmentConfig& config);
    /**
     * @brief Selects the order 1, ..., nAllotments.
     *
     * @return Vector with order <1, ..., nAllotments>.
     */
    virtual std::vector<unsigned> selectOrder() const override final;
    /**
     * @brief Getter for the name of the trivial sorter.
     *
     * @return The name of the trivial sorter.
     */
    virtual std::string getName() const override final;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~TrivialSorter() {};

private:
    /// @brief Manager for Input Data.
    ConstInputManagerPtr inputManager;
};


/**
 * @brief Sorter to order allotments by total expected profit.
 */
class ByTotalExpectedProfit: public AbstractAllotmentSorter {
public:
    /**
     * @brief Constructor for ByTotalExpectedProfit.
     *
     * @param config Configuration with input and links managers.
     */
    ByTotalExpectedProfit(const BaselineAllotmentConfig& config);
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ByTotalExpectedProfit() {};

private:
    /**
     * @brief Computes the total expected profit for the allotment.
     *
     * @param allotmentId The index of the allotment.
     * @return The total expected profit for the allotment.
     */
    virtual double getAllotmentMetric(unsigned allotmentId) const override final;
};

/**
 * @brief Sorter to order allotments by unit expected profit.
 */
class ByUnitExpectedProfit: public AbstractAllotmentSorter {
public:
    /**
     * @brief Constructor for ByUnitExpectedProfit.
     *
     * @param config Configuration with input and links managers.
     */
    ByUnitExpectedProfit(const BaselineAllotmentConfig& config);
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ByUnitExpectedProfit() {};

private:
    /**
     * @brief Computes the unit expected profit for the allotment.
     *
     * @param allotmentId The index of the allotment.
     * @return The unit expected profit for the allotment.
     */
    virtual double getAllotmentMetric(unsigned allotmentId) const override final;
};


/**
 * @brief Sorter to order allotments by total expected
 * shipped capacity (if booked, accepted, and not declined).
 */
class ByTotalExpectedCapacity: public AbstractAllotmentSorter {
public:
    /**
     * @brief Constructor for ByTotalExpectedCapacity.
     *
     * @param config Configuration with input and links managers.
     */
    ByTotalExpectedCapacity(const BaselineAllotmentConfig& config);
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ByTotalExpectedCapacity() {};

private:
    /**
     * @brief Computes the expected shipped capacity if
     * booked, accepted and not declined for a particular allotment.
     *
     * @param allotmentId The index of the allotment.
     * @return The described above capacity.
     */
    virtual double getAllotmentMetric(unsigned allotmentId) const override final;
};


/**
 * @brief Sorter to order allotments by total
 * capacity (if booked, accepted, all arrive, no declines).
 */
class ByTotalOptimisticCapacity: public AbstractAllotmentSorter {
public:
    /**
     * @brief Constructor for ByTotalOptimisticCapacity.
     *
     * @param config Configuration with input and links managers.
     */
    ByTotalOptimisticCapacity(const BaselineAllotmentConfig& config);
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ByTotalOptimisticCapacity() {};

private:
    /**
     * @brief Computes the expected shipped if booked, accepted,
     * all arrive, and no declines for particular allotment.
     *
     * @param allotmentId The index of the allotment.
     * @return The described above capacity.
     */
    virtual double getAllotmentMetric(unsigned allotmentId) const override final;
};

/**
 * @brief Sorter to order allotments by weighted show rate.
 */
class ByWeightedShowRate: public AbstractAllotmentSorter {
public:
    /**
     * @brief Constructor for ByWeightedShowRate.
     *
     * @param config Configuration with input and links managers.
     */
    ByWeightedShowRate(const BaselineAllotmentConfig& config);
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ByWeightedShowRate() {};

private:
    /**
     * @brief Computes the weighted show rate for a particular allotment.
     *
     * @param allotmentId The index of the allotment.
     * @return The described above value.
     */
    virtual double getAllotmentMetric(unsigned allotmentId) const override final;
};

/**
 * @brief Sorter to order allotments by weighted price.
 */
class ByWeightedPrice: public AbstractAllotmentSorter {
public:
    /**
     * @brief Constructor for ByWeightedPrice.
     *
     * @param config Configuration with input and links managers.
     */
    ByWeightedPrice(const BaselineAllotmentConfig& config);
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ByWeightedPrice() {};

private:
    /**
     * @brief Computes the weighted price for a particular allotment.
     *
     * @param allotmentId The index of the allotment.
     * @return The described above value.
     */
    virtual double getAllotmentMetric(unsigned allotmentId) const override final;
};

/**
 * @brief Sorter to order allotments by total optimistic (in sense all shows) profit.
 */
class ByTotalOptimisticProfit: public AbstractAllotmentSorter {
public:
    /**
     * @brief Constructor for ByTotalOptimisticProfit.
     *
     * @param config Configuration with input and links managers.
     */
    ByTotalOptimisticProfit(const BaselineAllotmentConfig& config);
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ByTotalOptimisticProfit() {};

private:
    /**
     * @brief Computes the total optimistic profit (when all
     * demand shows) for a particular allotment.
     *
     * @param allotmentId The index of the allotment.
     * @return The described above value.
     */
    virtual double getAllotmentMetric(unsigned allotmentId) const override final;
};


/**
 * @brief Entity to compute estimated profit proxy of the allotment order,
 * assuming the further processing on first-come first-served basis.
 */
class EstimatedProfitMetric: public IAllotmentOrderMetric {
public:
    /**
     * @brief Constructor for EstimatedProfitMetric.
     *
     * @param config Configuration with input and links managers.
     */
    EstimatedProfitMetric(const BaselineAllotmentConfig& config);
    /**
     * @brief Computes the total expected profit proxy for the allotment order.
     * In process, accounts for the allocated in-progress capacity by
     * tracking with local BaselineStats.
     *
     * @param allotmentOrder The order of allotments.
     * @return The computed total expected profit proxy for the allotment order.
     */
    virtual double score(const std::vector<unsigned>& allotmentOrder) const override final;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~EstimatedProfitMetric() {};

private:
    /// @brief Manager for Input Data.
    ConstInputManagerPtr inputManager;
    /// @brief Manager for Input Links.
    ConstLinksManagerPtr linksManager;
};


/** Composite sorter for allotments. Applies multiple sorters,
 * and selects the order with the highest metric value.
 */
class LongCompositeSorter: public IAllotmentSorter {
public:
    /**
     * @brief Constructor for LongCompositeSorter.
     *
     * @param config Configuration with input and links managers.
     */
    LongCompositeSorter(const BaselineAllotmentConfig& config);
    /**
     * @brief Selects the order by sequentially applying different
     * sorters, and selects the order with the highest metric value.
     *
     * @return Vector with the selected order of allotments.
     */
    virtual std::vector<unsigned> selectOrder() const override final;
    /**
     * @brief Getter for the name of the composite sorter.
     *
     * @return The name of the composite sorter.
     */
    virtual std::string getName() const override final;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~LongCompositeSorter() {};

private:
    /**
     * @brief Logs the score and order provided by a particular sorter.
     *
     * @param allotmentOrder The order of allotment indices.
     * @param sorterName The name of the sorter providing the allotment order.
     * @param reverseOrder Whether the original or reversed sorter output is considered.
     * @param score The metric value of the order.
     */
    void logSortScore(
            const std::vector<unsigned>& allotmentOrder,
            const std::string& sorterName,
            bool reverseOrder,
            double score) const;
private:
    /// @brief Manager for Input Data.
    ConstInputManagerPtr inputManager;
    /// @brief Manager for Input Links.
    ConstLinksManagerPtr linksManager;
    /// @brief Metric to compute the score of the allotment order.
    std::unique_ptr<IAllotmentOrderMetric> metric;
    /// @brief Vector of sorters to apply.
    std::vector<std::unique_ptr<IAllotmentSorter>> sorters;
};

} // namespace long
} // namespace backend
} // namespace sea

