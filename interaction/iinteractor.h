#pragma once

#include <memory>

#include "../manager.h"

namespace sea {

class IInteractor {
public:
    IInteractor() {};
    virtual void submitAction(ConstActionManagerPtr) = 0;
    virtual ConstDecisionManagerPtr makeDecision() = 0;
    virtual ~IInteractor() {};
};


}   // namespace sea
