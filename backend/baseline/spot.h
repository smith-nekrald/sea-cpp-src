#pragma once

#include "interface.h"
#include "../../manager.h"

#include <string>

namespace sea {
namespace backend {
namespace spot {

struct SpotSorterConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};


class AbstractSpotSorter : public ISpotSorter {
public:
    AbstractSpotSorter(const SpotSorterConfig& config, const std::string& sorterName);
    virtual std::vector<unsigned> selectOrder(
            const InputData::Event& event, const BaselineStats& stats) const override;
    virtual std::string getName() const override final;
    virtual ~AbstractSpotSorter() {};

protected:
    virtual double getRouteMetric(const InputData::Event& event, const BaselineStats& stats,
            Demand& demand, unsigned idxItinerary) const = 0;

private:
    void logMetricValues(
            const std::vector<unsigned>& itineraryIds,
            const std::vector<double>& metricValues) const;

private:
    const std::string name;

protected:
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};


} // namespace spot
} // namespace backend
} // namespace sea
