#pragma once

#include "interface.h"
#include "../../manager.h"

#include <string>
#include <map>

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
    virtual double getRouteMetric(
            const BaselineStats& stats, Demand& demand, unsigned idxItinerary) const = 0;

private:
    void logMetricValues(
            const InputData::Event& event,
            const std::map<unsigned, double>& metricValues) const;

private:
    const std::string name;

protected:
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};

class ByExpectedTotalProfit : public AbstractSpotSorter {
public:
    ByExpectedTotalProfit(const SpotSorterConfig& config);

private:
    virtual double getRouteMetric(
            const BaselineStats& stats,
            Demand& demand,
            unsigned idxItinerary) const override final;

};

class TrivialItineraryOrder : public ISpotSorter {
public:
    TrivialItineraryOrder();
    virtual std::vector<unsigned> selectOrder(
            const InputData::Event& event, const BaselineStats& stats) const override final;
    virtual std::string getName() const override final;
    virtual ~TrivialItineraryOrder() {};
};

class CompositeSpotSorter : public ISpotSorter {
public:
    CompositeSpotSorter(const SpotSorterConfig& config);
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
    SpotEventProfitMetric(const SpotSorterConfig& config);
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
