#pragma once

#include "baseline_stats.h"
#include "../../input/input_data.h"

#include <vector>
#include <string>

namespace sea {
namespace backend {


class IAllotmentSorter {
public:
    IAllotmentSorter() {};
    virtual std::vector<unsigned> selectOrder() const = 0;
    virtual std::string getName() const = 0;
    virtual ~IAllotmentSorter() {};
};

class IAllotmentOrderMetric {
public:
    IAllotmentOrderMetric() {};
    virtual double score(const std::vector<unsigned>& allotmentOrder) const = 0;
    virtual ~IAllotmentOrderMetric() {};
};

class ISpotSorter {
public:
    ISpotSorter() {};
    virtual std::vector<unsigned> selectOrder(
            const InputData::Event& event, const BaselineStats& stats) const = 0;
    virtual std::string getName() const = 0;
    virtual ~ISpotSorter() {};
};

class ISpotOrderMetric {
public:
    ISpotOrderMetric() {};
    virtual double score(const std::vector<unsigned>& spotOrder,
            const InputData::Event& event, const BaselineStats& stats) const = 0;
    virtual ~ISpotOrderMetric() {};
};

} // namespace backend
} // namespace sea
