/**
 * @file launcher.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines Launcher. Launcher is an entity to run a set of algorithms on a set of 
 * market trajectory realizations and collect the evaluation statistics.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "evaluator.h"
#include "../manager.h"

#include <vector>
#include <string>
#include <map>

namespace sea {

/**
 * @brief Configures launcher.
 */
struct LauncherConfig {
    /// @brief The algorithms to launch.
    std::vector<algo::IAlgorithmPtr> algorithms;
    /// @brief The paths to the files with market data to evaluate on.
    std::vector<std::string> marketDataSetPaths;
    /// @brief Manager to the InputData structure.
    ConstInputManagerPtr inputManager;
    /// @brief Manager to the InputLinks structure.
    ConstLinksManagerPtr linksManager;
    /// @brief If true, performs careful RAM memory management.
    std::experimental::optional<bool> needMemory;
    /// @brief If true, tends to collect history in evaluator and algorithm.
    std::experimental::optional<bool> writeStory;
};

/**
 * @brief Launches a set of algorithms on a set of market data and collects evaluation 
 * statistics. Essentially, calls Evaluator API for each algorithm and market data.
 */
class Launcher {
public:
    /**
     * @brief Launcher constructor. 
     * 
     * @param aConfig Configuration for launches.
     */
    explicit Launcher(const LauncherConfig& aConfig);
    /**
     * @brief Runs all configured launches.
     * 
     * @return Map from algorithm name to a vector of evaluation statistics, each entry
     * corresponds to a market data realization (in the same order for each algorithm).
     */
    std::map<std::string, std::vector<Statistics>> doLaunches();

private:
    /**
     * @brief Logs that launcher is initializer.
     */
    void logLauncherInitialization() const;
    /**
     * @brief Logs that launcher has started evaluation of algorithm algoName on market data
     * with index idxMarket.
     * 
     * @param algoName The name of the algorithm launched.
     * @param idxMarket The index of the market data trajectory evaluated.
     */
    void logStartedEvaluation(std::string algoName, unsigned idxMarket) const;
    /**
     * @brief Logs that launcher has finished evaluation.
     */
    void logFinishedEvaluation() const;
    /**
     * @brief Logs that market data is read from file.
     * 
     * @param dataPath The file path to read the market data from.
     */
    void logReadMarketDataset(const std::string& dataPath) const;

private:
    /// @brief Configuration for launches.
    LauncherConfig config;
};

} // namespace sea
