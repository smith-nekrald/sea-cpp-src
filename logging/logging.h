/**
 * @file logging.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Declares methods to retrieve various loggers.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../common.h"

#include <string>
#include <iostream>

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

namespace logging {

/**
 * @brief Configures logger according to the configuration in the file.
 * 
 * @param configPath Path to the file with configuration.
 */
void configureLogger(const std::string configPath);

/**
 * @brief Getter for the root logger.
 * 
 * @return The root-level logger.
 */
log4cpp::Category& getRootLogger();

/**
 * @brief Get the Evaluation Logger.
 * 
 * @return Evaluation logger.
 */
log4cpp::Category& getEvaluationLogger();

/**
 * @brief Get the Memory Logger.
 * 
 * @return Memory logger.
 */
log4cpp::Category& getMemoryLogger();


/**
 * @brief Get the Algorithm Logger.
 * 
 * @return Algorithm logger.
 */
log4cpp::Category& getAlgorithmLogger();
/**
 * @brief Get the Strategy Logger.
 * 
 * @return Strategy logger.
 */
log4cpp::Category& getStrategyLogger();
/**
 * @brief Get the Allotment Strategy Logger.
 * 
 * @param type Type of allotment strategy.
 * @return Allotment Strategy Logger.
 */
log4cpp::Category& getAllotmentStrategyLogger(const sea::AllotmentStrategyType& type);
/**
 * @brief Get the Spot Strategy Logger.
 * 
 * @param type Type of spot strategy.
 * @return Spot Strategy Logger.
 */
log4cpp::Category& getSpotStrategyLogger(const sea::SpotStrategyType& type);

/**
 * @brief Get the Backend Logger.
 * 
 * @return Backend Logger.
 */
log4cpp::Category& getBackendLogger();
/**
 * @brief Get the Backend Sublogger.
 * 
 * @param type Type of Backend.
 * @return The corresponding Backend sublogger.
 */
log4cpp::Category& getBackendSubLogger(const sea::BackendType& type);

/**
 * @brief Get the Validation Logger.
 * 
 * @return The validation logger.
 */
log4cpp::Category& getValidationLogger();

/**
 * @brief Get the Out Test Logger.
 * 
 * @return The testing logger.
 */
log4cpp::Category& getOutTestLogger();

} // namespace logging
