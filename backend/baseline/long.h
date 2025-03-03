#pragma once

#include "interface.h"
#include "../../manager.h"

#include <memory>

namespace sea {
namespace backend {
namespace allotment {

struct AllotmentSorterConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};


class AbstractAllotmentSorter: public IAllotmentSorter {
public:
    AbstractAllotmentSorter(const AllotmentSorterConfig& config, const std::string& sorterName);
    virtual std::vector<unsigned> selectOrder() const override;
    virtual std::string getName() const override final;
    virtual ~AbstractAllotmentSorter() {};

protected:
    virtual double getAllotmentMetric(unsigned allotmentId) const = 0;

private:
    void logMetricValues(const std::vector<double>& metricValues) const;

private:
    std::string name;

protected:
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};


class TrivialSorter: public IAllotmentSorter {
public:
    TrivialSorter(const AllotmentSorterConfig& config);
    virtual std::vector<unsigned> selectOrder() const override;
    virtual std::string getName() const override final;
    virtual ~TrivialSorter() {};

private:
    ConstInputManagerPtr inputManager;
};


class ByTotalExpectedProfit: public AbstractAllotmentSorter {
public:
    ByTotalExpectedProfit(const AllotmentSorterConfig& config);
    virtual ~ByTotalExpectedProfit() {};

private:
    virtual double getAllotmentMetric(unsigned allotmentId) const;
};


class ByUnitExpectedProfit: public AbstractAllotmentSorter {
public:
    ByUnitExpectedProfit(const AllotmentSorterConfig& config);
    virtual ~ByUnitExpectedProfit() {};

private:
    virtual double getAllotmentMetric(unsigned allotmentId) const;
};


class EstimatedProfitMetric: public IAllotmentOrderMetric {
public:
    EstimatedProfitMetric(const AllotmentSorterConfig& config);
    virtual double score(const std::vector<unsigned>& allotmentOrder) const override final;
    virtual ~EstimatedProfitMetric() {};

private:
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};


class LongCompositeSorter: public IAllotmentSorter {
public:
    LongCompositeSorter(const AllotmentSorterConfig& config);
    virtual std::vector<unsigned> selectOrder() const override final;
    virtual std::string getName() const override final;
    virtual ~LongCompositeSorter() {};

private:
    void logSortScore(
            const std::vector<unsigned>& allotmentOrder,
            const std::string& sorterName,
            bool reverseOrder,
            double score) const;
private:
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
    std::unique_ptr<IAllotmentOrderMetric> metric;
    std::vector<std::unique_ptr<IAllotmentSorter>> sorters;
};


} // namespace long
} // namespace backend
} // namespace sea

