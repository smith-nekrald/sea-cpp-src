// Logging methods.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include <jsoncpp/json/json.h>

#include "start.h"
#include "logging/logging.h"

void logNoIpoptBackendConfiguration() {
        logging::getRootLogger().error("No Ipopt Backend configuration in JSON.");
}

void logNoLagrangianBackendConfiguration() {
    logging::getRootLogger().warn("No configuration in JSON for Lagrangian Relaxation Backend.");
}

void logNoBendersBackendConfiguration() {
    logging::getRootLogger().warn("No configuration for Benders Backend in JSON.");
}

void logNoDCPBackendConfiguration() {
    logging::getRootLogger().warn("No configuration for DCP backend in JSON.");
}

void logFinishedReadingMarket(const std::string& dataPath) {
    logging::getRootLogger().info("Finished reading market for dataPath: " + dataPath);
}

void logCreatedSpotMarketStrategies() {
    logging::getRootLogger().info("Spot market strategies created.");
}

void logCreatedAllotmentStrategies() {
    logging::getRootLogger().info("Allotment strategies created.");
}

void logReadyToLaunch() {
    logging::getRootLogger().info("Ready to launch.");
}

void logLaunchFinished() {
    logging::getRootLogger().info("Launch have finished.");
}

void logReadyToAnalyze() {
    logging::getRootLogger().info("Ready to analyze.");
}

void logAnalysisFinished() {
    logging::getRootLogger().info("Finished analysis");
}

void logStartedWithConfig(const std::string& pathToLoggingConfig) {
    logging::getRootLogger().info("Started with config: " + pathToLoggingConfig);
}

void logMarketDataSearch(
        const std::string& dataPath, const std::vector<std::string>& marketDataFiles) {
    logging::getRootLogger().info("Looking for market data files in " + dataPath);
    logging::getRootLogger().info(
            "Processing " + std::to_string(marketDataFiles.size()) +  " market data sets.");
    logging::getRootLogger().info("Finished reading market");
}

void logDoneReadInput() {
    logging::getRootLogger().info("Finished reading input.");
}

void logDoneWithLinks() {
    logging::getRootLogger().info("Built links based on input.");
}

void logCreatedAlgorithm(const std::string& algoName) {
    logging::getRootLogger().info("Created algorithm with name: " + algoName);
}
