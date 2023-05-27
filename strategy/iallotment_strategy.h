/**
 * @file iallotment_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares Allotment Strategy Interface.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../manager.h"
#include "../backend/backend_general.h"

#include <memory>
#include <string>

namespace sea {
namespace strategy {

/**
 * @brief Interface to an entity for making allotment decision.
 * The main method and entry point is called provideAllotments().
 */
class IAllotmentStrategy {
public:
    /**
     * @brief Constructor for a new IAllotmentStrategy.
     */
    IAllotmentStrategy() {};

    /**
     * @brief Creates new DecisionManagerPtr and fills field related to selected allotments.
     *
     * @return Decision Manager with filled selected allotments.
     */
    virtual DecisionManagerPtr provideAllotments() = 0;
    /**
     * @brief Returns the name of the allotment strategy.
     *
     * @return Name of the allotment strategy.
     */
    virtual std::string getName() = 0;

    /**
     * @brief Resets the strategy.
     */
    virtual void reset() = 0;
    /**
     * @brief Hard-Resets the strategy. Resets backends and then calls reset().
     */
    virtual void hardReset() = 0;

    /**
     * @brief Sets the maximal capacity utilization ratio.
     *
     * @param ratio Maximal capacity utilization ratio.
     */
    virtual void setUtilizationRatio(double ratio) = 0;
    /**
     * @brief Retrieves maximal capacity utilization ratio.
     *
     * @return The maximal capacity utilization ratio.
     */
    virtual double getUtilizationRatio() = 0;

    /**
     * @brief Getter for the backend holder.
     *
     * @return Backend Holder with relevant backends.
     */
    virtual BackendHolder getBackends() = 0;
    /**
     * @brief Getter for the objective value estimation.
     *
     * @return The future value estimation.
     */
    virtual double getValueEstimation() = 0;

    /**
     * @brief Virtual destructor for correct C++ polymorphism.
     */
    virtual ~IAllotmentStrategy() {};
};

/// @brief Simple name for the shared pointer to the allotment strategy.
using IAllotmentStrategyPtr=std::shared_ptr<IAllotmentStrategy>;

} // namespace strategy
} // namespace sea
