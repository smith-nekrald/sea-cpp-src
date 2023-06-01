/**
 * @file dcp_allotment_strategy.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Allotment Strategy, based on Deterministic Cutting Plane method.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../abstract_allotment_strategy.h"
#include "../../backend/det_cut_plane/dcp_backend.h"

namespace sea {
namespace strategy {

/**
 * @brief Algorithm-specific configuration. No entries at this version, but who knows,
 * maybe this gets expanded at some time point.
 */
struct DCPInnerConfig {
};

/**
 * @brief Configures deterministic cutting plane allotment strategy.
 */
struct DetCutPlaneAllotmentStrategyConfig {
    /// @brief Generic allotment strategy configuration.
    AllotmentStrategyConfig abstractConfig;
    /// @brief Holder for backend configurations.
    BackendConfigHolder backendConfigs;
    /// @brief Algorithm-specific configuration.
    DCPInnerConfig innerConfig;
};

/**
 * @brief Allotment Strategy, based on the Deterministic Cutting Plane method.
 * Essentially, calls DetCutPlane backend and provides correct interface.
 */
class DetCutPlaneAllotmentStrategy : public AbstractAllotmentStrategy {
public:
    /**
     * @brief Constructor for Deterministic Cutting Plane Allotment Strategy.
     * 
     * @param aConfig Configuration for DCP strategy.
     */
    DetCutPlaneAllotmentStrategy(const DetCutPlaneAllotmentStrategyConfig& aConfig)
        : AbstractAllotmentStrategy(
                aConfig.abstractConfig, aConfig.backendConfigs, "det_cut_plane_allotment")
        , innerConfig(aConfig.innerConfig) {}

    /**
     * @brief Decides on the allotments to select.
     * 
     * @return Decision Manager with written allotments to select.
     */
    virtual DecisionManagerPtr provideAllotments() override;
    /**
     * @brief Soft reset. Resets inner algorithm state without resetting backends.
     */
    virtual void reset() override;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~DetCutPlaneAllotmentStrategy() {}

private:
    /**
     * @brief Logs answer obtained in provideAllotments.
     * @param decision The decision to log.
     */
    void logPrintAnswerInProvideAllotments(ConstDecisionManagerPtr decision) const;

private:
    /// @brief Algorithm-specific configuration.
    const DCPInnerConfig innerConfig;
};

} // namespace strategy
} // namespace sea
