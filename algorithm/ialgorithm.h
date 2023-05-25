/**
 * @file ialgorithm.h
 * @brief Specifies algorithm interface, called IAlgorithm.
 * 
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../interaction/iinteractor.h"

#include <string>

namespace sea {
namespace algo {

/**
 * @brief Algorithm interface. Algorithm is supposed to make decisions and accept 
 * corresponding environment actions.
 */
class IAlgorithm : public IInteractor {
public:
    /**
    * @brief Constructor. Since interface, needs overloading.
    */
    IAlgorithm() {};

    /**
     * @brief Getter or builder for the algorithm name.
     * 
     * @return The name of the algorithm. 
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Supplies algorithm with an action. Action is essentially the 
     * environment response or update subject to the previously made decisions.
     * 
     * @param action The pointer to a manager object carrying action.
     */
    virtual void submitAction(ConstActionManagerPtr action) override = 0;

    /**
     * @brief Mades a decision. Decision entity is essentially the algorithm 
     * response to the next event that requires some decision.
     * 
     * @return The smart pointer to a manager object carrying the decision.
     */
    virtual ConstDecisionManagerPtr makeDecision() override = 0;

    /**
     * @brief A method to reset the inner algorithm state. Helps when doing
     * optimization for a new market realization.
     */
    virtual void reset() = 0;

    /**
     * @brief Sometimes algorithm is built from strategies, and these strategies 
     * support synchronization. E.g. Ipopt allotment strategy uses the same fluid 
     * approximation as Ipopt spot strategy, and therefore can share the optimization
     * resuls to accelerate optimization process. Another example is propagating some
     * configuration parameter from allotment model to spot model.
     */
    virtual void synchronizeStrategies() = 0;
    
    /**
     * @brief If the story tracking is configured, returns some historic information
     * collected during the algorithm execution. E.g. value estimations, intermediate 
     * decisions, etc.
     * 
     * @return The structure mapping story keys to vector of story-related double values.
     */
    virtual std::map<std::string, std::vector<double>> getStory() const = 0;

    /**
     * @brief Destroys the IAlgorithm object. Virtual destructor, to make
     * C++ polymorphism work reasonable.
     */
    virtual ~IAlgorithm() {};
};

/// @brief Simplified notation for the shared smart-pointer to an algorithm object.
typedef std::shared_ptr<IAlgorithm> IAlgorithmPtr;

} //namespace algo
} //namespace sea
