#pragma once

#include "interface.h"
#include "../../input/input_data.h"
#include "../../input/input_links.h"
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
    AbstractAllotmentSorter(const AllotmentSorterConfig& config);
    virtual std::vector<unsigned> selectOrder() const override;
    virtual ~AbstractAllotmentSorter() {};

protected:
    virtual double getAllotmentMetric(unsigned allotmentId) const = 0;

protected:
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
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
    virtual ~LongCompositeSorter() {};

private:
    std::vector<std::unique_ptr<IAllotmentSorter>> sorters;
    std::unique_ptr<IAllotmentOrderMetric> metric;
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};



} // namespace long
} // namespace backend
} // namespace sea

