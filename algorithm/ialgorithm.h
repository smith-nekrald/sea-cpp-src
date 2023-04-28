#pragma once

#include "../interaction/iinteractor.h"

#include <string>

namespace sea {
namespace algo {

class IAlgorithm : public IInteractor {
public:
    IAlgorithm() {};
    virtual std::string getName() = 0;
    virtual void submitAction(ConstActionManagerPtr) override = 0;
    virtual ConstDecisionManagerPtr makeDecision() override = 0;
    virtual void reset() = 0;
    virtual void synchronizeStrategies() = 0;
    virtual std::map<std::string, std::vector<double>> getStory() const = 0;
    virtual ~IAlgorithm() {};
};

typedef std::shared_ptr<IAlgorithm> IAlgorithmPtr;

} //namespace algo
} //namespace sea
