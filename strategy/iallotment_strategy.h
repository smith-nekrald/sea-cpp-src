#pragma once

#include "../manager.h"
#include "../backend/backend_general.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

class IAllotmentStrategy {
public:
    IAllotmentStrategy() {};
    // Creates new DecisionManagerPtr and fills field related to selected allotments.
    virtual DecisionManagerPtr provideAllotments() = 0;
    virtual std::string getName() = 0;
    virtual void reset() = 0;
    virtual void hardReset() = 0;

    virtual void setUtilizationRatio(double ratio) = 0;
    virtual double getUtilizationRatio() = 0;

    virtual BackendHolder getBackends() = 0;
    virtual double getValueEstimation() = 0;

    virtual ~IAllotmentStrategy() {};
};

using IAllotmentStrategyPtr=std::shared_ptr<IAllotmentStrategy>;

} // namespace strategy
} // namespace sea
