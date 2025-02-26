#pragma once

#include "interface.h"
#include "../../input/input_data.h"
#include "../../input/input_links.h"
#include "../../manager.h"

#include <memory>

namespace sea {
namespace backend {

struct AllotmentSorterConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};

class ByTotalProfit: public IAllotmentSorter {
public:
    ByTotalProfit(const AllotmentSorterConfig& config);
    virtual std::vector<unsigned> selectOrder() override final;
    virtual ~ByTotalProfit() {};

private:
    double getTotalProfit(unsigned allotmentId) const;

private:
    const InputData& input;
    const InputLinks& links;
};


class EstimatedProfitMetric: public IAllotmentOrderMetric {
public:
    EstimatedProfitMetric(const AllotmentSorterConfig& config);
    virtual double score(const std::vector<unsigned>& allotmentOrder) override final;
    virtual ~EstimatedProfitMetric() {};

private:
    const InputData& input;
    const InputLinks& links;
};

class LongCompositeSorter: public IAllotmentSorter {
public:
    LongCompositeSorter(const AllotmentSorterConfig& config);
    virtual std::vector<unsigned> selectOrder() override final;
    virtual ~LongCompositeSorter() {};

private:
    const InputData& input;
    const InputLinks& links;
    std::vector<std::unique_ptr<IAllotmentSorter>> sorters;
    std::unique_ptr<IAllotmentOrderMetric> metric;
};




} // namespace sea
} // namespace backend

