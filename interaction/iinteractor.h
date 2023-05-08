/**
 * @file iinteractor.h
 * @brief Speficies IInteractor - the interface of interaction between policy and environment.
 * 
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @copyright (c) Smith School of Business, 2023
 */

#pragma once

#include <memory>

#include "../manager.h"

namespace sea {

/**
 * @brief Interface describing interaction with policy. Essentially, any policy is Interactor.
 * Interactor makes decisions (via makeDecision) and receives environment actions 
 * (via submitAction).
 */
class IInteractor {
public:
    /**
     * @brief Constructor. Expects overriding.
     */
    IInteractor() {};

    /**
     * @brief Supplies algorithm with an action. Action is essentially the 
     * environment response or update subject to the previously made decisions.
     * Virtual method, needs redefinition.
     * 
     * @param action Provides a smart pointer to the Action holder 
     */
    virtual void submitAction(ConstActionManagerPtr action) = 0;

    /**
     * @brief Responsible for making a decision. Virtual method, needs redefinition.
     * 
     * @return Smart pointer on an object that holds Decision.
     */
    virtual ConstDecisionManagerPtr makeDecision() = 0;

    /**
     * @brief Virtual destructor. To ensure that C++ polymorphism works properly.
     */
    virtual ~IInteractor() {};
};


}   // namespace sea
