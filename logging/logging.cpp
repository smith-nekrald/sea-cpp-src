#include "logging.h"

namespace logging {

void configureLogger(const std::string configPath) {
    log4cpp::PropertyConfigurator::configure(configPath);
}

log4cpp::Category& getRootLogger() {
    return log4cpp::Category::getRoot();
}

log4cpp::Category& getEvaluationLogger() {
    return log4cpp::Category::getInstance(
            std::string("evaluationLogger"));
}

log4cpp::Category& getMemoryLogger() {
    return log4cpp::Category::getInstance(
            std::string("memoryLogger"));
}

log4cpp::Category& getAlgorithmLogger() {
    return log4cpp::Category::getInstance(
            std::string("algorithmLogger"));
}

log4cpp::Category& getStrategyLogger() {
    return log4cpp::Category::getInstance(
            std::string("algorithmLogger.strategyLogger"));
}

log4cpp::Category& getBackendLogger() {
    return log4cpp::Category::getInstance(
            std::string("backendLogger"));
}

log4cpp::Category& getValidationLogger() {
    return log4cpp::Category::getInstance(
            std::string("validationLogger"));
}

log4cpp::Category& getOutTestLogger() {
    return log4cpp::Category::getInstance(
            std::string("outTestLogger"));
}
log4cpp::Category& getAllotmentStrategyLogger(const sea::AllotmentStrategyType& type) {
    if (type == sea::AllotmentStrategyType::IPOPT) {
        return log4cpp::Category::getInstance(std::string(
            "algorithmLogger.strategyLogger.ipoptAllotmentStrategyLogger"));
    } else if (type == sea::AllotmentStrategyType::BENDERS) {
        return log4cpp::Category::getInstance(std::string(
            "algorithmLogger.strategyLogger.bendersAllotmentStrategyLogger"));
    } else if (type == sea::AllotmentStrategyType::DET_CUT_PLANE) {
        return log4cpp::Category::getInstance(std::string(
            "algorithmLogger.strategyLogger.detCutPlaneAllotmentStrategyLogger"));
    } else if (type == sea::AllotmentStrategyType::ZERO_IPOPT) {
        return log4cpp::Category::getInstance(std::string(
            "algorithmLogger.strategyLogger.zeroAllotmentStrategyLogger"));
    } else if (type == sea::AllotmentStrategyType::ZERO_NULL) {
        return log4cpp::Category::getInstance(std::string(
            "algorithmLogger.strategyLogger.nullAllotmentStrategyLogger"));
    } else {
        throw std::logic_error("Unknown AllotmentStrategyType.");
    }
}

log4cpp::Category& getSpotStrategyLogger(const sea::SpotStrategyType& type) {
    if (type == sea::SpotStrategyType::IPOPT) {
        return log4cpp::Category::getInstance(std::string(
            "algorithmLogger.strategyLogger.ipoptSpotStrategyLogger"));
    } else if (type == sea::SpotStrategyType::LR) {
        return log4cpp::Category::getInstance(std::string(
            "algorithmLogger.strategyLogger.lrSpotStrategyLogger"));
    } else if (type == sea::SpotStrategyType::HYBRID) {
        return log4cpp::Category::getInstance(std::string(
            "algorithmLogger.strategyLogger.hybridSpotStrategyLogger"));
    } else {
        throw std::logic_error("Unknown SpotStrategyType.");
    }
}

log4cpp::Category& getBackendSubLogger(const sea::BackendType& type) {
    if (type == sea::BackendType::LR) {
        return log4cpp::Category::getInstance(std::string(
            "backendLogger.lrBackendLogger"));
    } else if (type == sea::BackendType::DET_CUT_PLANE) {
        return log4cpp::Category::getInstance(std::string(
            "backendLogger.detCutPlaneBackendLogger"));
    } else if (type == sea::BackendType::IPOPT) {
        return log4cpp::Category::getInstance(std::string(
            "backendLogger.ipoptBackendLogger"));
    } else if (type == sea::BackendType::BENDERS) {
        return log4cpp::Category::getInstance(std::string(
            "backendLogger.bendersBackendLogger"));
    } else  {
        throw std::logic_error("Unknown BackendType.");
    }
}

} // namespace logging
