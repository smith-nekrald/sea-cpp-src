#pragma once

#include "../common.h"

#include <string>
#include <iostream>

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

namespace logging {

void configureLogger(const std::string configPath);
log4cpp::Category& getRootLogger();

log4cpp::Category& getEvaluationLogger();

log4cpp::Category& getMemoryLogger();

log4cpp::Category& getAlgorithmLogger();
log4cpp::Category& getStrategyLogger();
log4cpp::Category& getAllotmentStrategyLogger(const sea::AllotmentStrategyType& type);
log4cpp::Category& getSpotStrategyLogger(const sea::SpotStrategyType& type);

log4cpp::Category& getBackendLogger();
log4cpp::Category& getBackendSubLogger(const sea::BackendType& type);

log4cpp::Category& getValidationLogger();

log4cpp::Category& getOutTestLogger();

} // namespace logging
