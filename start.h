/**
 * @file start.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements configuration parsing and related API.
 * The entry point is the function start().
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <chrono>
#include <jsoncpp/json/json.h>

#include "common.h"
#include "manager.h"
#include "algorithm/strategic_algorithm.h"
#include "backend/backend_general.h"
#include "backend/lagrangian_relaxation/gm_sea/gm_src/gm_types.h"
#include "strategy/general_strategy.h"
#include "reader/reader_general.h"
#include "evaluation/evaluation_general.h"
#include "logging/logging.h"
#include "sense/validator.h"

namespace json=Json;
using std::size_t;

using sea::backend::IpoptBackendConfig;
using sea::backend::LagrangianRelaxationBackendConfig;
using sea::backend::BendersAllotmentBackendConfig;
using sea::backend::DetCutPlaneBackendConfig;

/**
 * @brief Reads IpoptBackendConfig from JSON root.
 *
 * @param configRoot The root of JSON configuration.
 * @param config The config to read.
 */
void fillBackendConfig(const json::Value& configRoot, IpoptBackendConfig& config);
/**
 * @brief Reads LagrangianRelaxation Backend Config from JSON root.
 *
 * @param configRoot The root of JSON configuration.
 * @param config The config to read.
 */
void fillBackendConfig(const json::Value& configRoot, LagrangianRelaxationBackendConfig& config);
/**
 * @brief Reads Benders Decomposition Backend Config from JSON root.
 *
 * @param configRoot The root of JSON configuration.
 * @param config The config to read.
 */
void fillBackendConfig(const json::Value& configRoot, BendersAllotmentBackendConfig& config);
/**
 * @brief Reads DCP backend config from JSON root.
 *
 * @param configRoot The root of JSON configuration.
 * @param config The config to read.
 */
void fillBackendConfig(const json::Value& configRoot, DetCutPlaneBackendConfig& config);

/**
 * @brief Looks for market data files in directory dataPath and creates a vector with those.
 *
 * @param dataPath The path to the directory to look for Market Data files.
 * @param count_limit Maximal number of files to return. Shrinks the vector if needed.
 * @return Vector with paths to market data files.
 */
std::vector<std::string> getMarketDataFiles(std::string dataPath, std::size_t count_limit=10000);
/**
 * @brief Reads InputData object from dataPath.
 *
 * @param dataPath The path to read the InputData from.
 * @param needMemory If true, resulting manager is configured for efficient memory management.
 * @return The Manager entity with InputData.
 */
sea::ConstInputManagerPtr getInput(std::string dataPath, bool needMemory);
/**
 * @brief Creates a Manager with InputLinks from InputData.
 *
 * @param inputManager A manager with InputData.
 * @param needMemory If true, resulting manager is configured for efficient memory management.
 * @return The Manager entity with InputLinks.
 */
sea::ConstLinksManagerPtr getLinks(
        const sea::ConstInputManagerPtr& inputManager, bool needMemory);
/**
 * @brief Reads all MarketData according to paths specified in pathVector.
 *
 * @param pathVector The vector with paths to MarketData files.
 * @param needMemory If true, resulting manager is configured for efficient memory management.
 * @return Vector with Managers for the read Market Data sets.
 */
std::vector<sea::ConstMarketManagerPtr> getMarketVector(
        const std::vector<std::string>& pathVector, bool needMemory);

/**
 * @brief Fills BackendConfigHolder from JSON configuration.
 *
 * @param[in] configRoot The root of JSON configuration.
 * @param[in] inputManager  Manager for already built InputData.
 * @param[in] linksManager  Manager for already built InputLinks.
 * @param[out] holderPtr The holder to fill.
 */
void prepareBackendConfigHolder(const json::Value& configRoot,
        sea::ConstInputManagerPtr inputManager,
        sea::ConstLinksManagerPtr linksManager,
        sea::BackendConfigHolder* holderPtr);
/**
 * @brief Prepares Spot Market Strategies. Fills spotStrategies[Ptr] and spotNames[Ptr].
 *
 * @param[in] configRoot The root of JSON configuration.
 * @param[in] holder The holder with backend configurations.
 * @param[in] inputManager Manager with already built InputData.
 * @param[in] linksManager  Manager with already built InputLinks.
 * @param[out] strategyConfigsPtr Strategy configurations.
 * @param[out] spotStrategiesPtr Map from spot strategy name to a corresponding entity.
 * @param[out] spotNamesPtr Vector with spot strategy names.
 */
void prepareSpotStrategies(
        const json::Value& configRoot, const sea::BackendConfigHolder& holder,
        sea::ConstInputManagerPtr inputManager, sea::ConstLinksManagerPtr linksManager,
        sea::StrategyConfigs* strategyConfigsPtr,
        std::map<std::string, sea::strategy::ISpotMarketStrategyPtr>* spotStrategiesPtr,
        std::vector<std::string>* spotNamesPtr);
/**
 * @brief Prepares Allotment Strategies. Fills allotmentStrategies[Ptr] and allotmentNames[Ptr].
 *
 * @param[in] configRoot The root of JSON configuration.
 * @param[in] holder The holder with backend configurations.
 * @param[in] inputManager Manager with already built InputData.
 * @param[in] linksManager Manager with already built InputLinks.
 * @param[out] strategyConfigsPtr Strategy configurations.
 * @param[out] allotmentStrategiesPtr Map from allotment strategy name to a corresponding entity.
 * @param[out] allotmentNamesPtr Vector with allotment strategy names.
 */
void prepareAllotmentStrategies(const json::Value& configRoot,
        const sea::BackendConfigHolder& holder,
        sea::ConstInputManagerPtr inputManager,
        sea::ConstLinksManagerPtr linksManager,
        sea::StrategyConfigs* strategyConfigsPtr,
        std::map<std::string, sea::strategy::IAllotmentStrategyPtr>* allotmentStrategiesPtr,
        std::vector<std::string>* allotmentNamesPtr);

/**
 * @brief Prepares configuration pairs, i.e. pairs of spot-allotment strategy names.
 *
 * @param[in] configRoot The root of JSON configuration.
 * @param[in] spotNames Names of spot market strategies.
 * @param[in] allotmentNames  Names of allotment market strategies.
 * @param[out] configurationPairsPtr The resulting spot-allotment pairs.
 */
void prepareConfigurationPairs(const json::Value& configRoot,
        const std::vector<std::string>& spotNames,
        const std::vector<std::string>& allotmentNames,
        std::vector<std::pair<std::string, std::string>>* configurationPairsPtr);

/**
 * @brief Starts the process of validation.
 *
 * @param marketDataFiles The MarketData file paths.
 * @param needMemory If true, efficient memory management is executed.
 * @param inputManager Manager with InputData.
 * @param linksManager Manager with InputLinks.
 * @param holder Holder for backend configurations.
 * @param strategyConfigs Holder for strategy configurations.
 */
void startValidation(const std::vector<std::string>& marketDataFiles, bool needMemory,
    sea::ConstInputManagerPtr inputManager, sea::ConstLinksManagerPtr linksManager,
    const sea::BackendConfigHolder& holder, const sea::StrategyConfigs& strategyConfigs);

/**
 * @brief Starts the experiments. Evaluates each algorithm on each market data, and
 * summarizes the performance of the algorithms.
 *
 * @param configRoot The root of JSON configuration.
 * @param marketDataFiles The vector with paths to market data files.
 * @param needMemory If true, managers are to use efficient memory management.
 * @param inputManager Manager with InputData.
 * @param linksManager Manager with InputLinks.
 * @param algoVector The vector with algorithm to evaluate.
 */
void startAlgorithms(const json::Value& configRoot,
        const std::vector<std::string> marketDataFiles, bool needMemory,
        sea::ConstInputManagerPtr inputManager, sea::ConstLinksManagerPtr linksManager,
        const std::vector<sea::algo::IAlgorithmPtr>& algoVector);
/**
 * @brief Entry point. Parses JSON configuration and executes accordingly.
 *
 * @param configRoot The root of JSON configuration.
 */
void start(const json::Value& configRoot);
/**
 * @brief Logs that JSON have no IPOPT backend configuration.
 */
void logNoIpoptBackendConfiguration();
/**
 * @brief Logs that JSON have no Lagrangian Relaxation backend configuration.
 */
void logNoLagrangianBackendConfiguration();
/**
 * @brief Logs that JSON have no configuration for Benders backend.
 */
void logNoBendersBackendConfiguration();
/**
 * @brief Logs that JSON have no configuration for DCP backend.
 */
void logNoDCPBackendConfiguration();
/**
 * @brief Logs that MarketData is read from specific path.
 *
 * @param dataPath The path the data was read from.
 */
void logFinishedReadingMarket(const std::string& dataPath);
/**
 * @brief Logs that spot market strategies are created.
 */
void logCreatedSpotMarketStrategies();
/**
 * @brief Logs that allotment strategies are created.
 */
void logCreatedAllotmentStrategies();
/**
 * @brief Logs that all configuration is done and launcher is ready for evaluations.
 */
void logReadyToLaunch();
/**
 * @brief Logs that launcher has finished.
 */
void logLaunchFinished();
/**
 * @brief Logs that analyzer is ready to start.
 */
void logReadyToAnalyze();
/**
 * @brief Logs that analysis is finished.
 */
void logAnalysisFinished();
/**
 * @brief Logs that the configuration is started with a particular config.
 *
 * @param pathToLoggingConfig Path to the JSON config that is parsed.
 */
void logStartedWithConfig(const std::string& pathToLoggingConfig);
/**
 * @brief Logs the process of market data search.
 *
 * @param dataPath The path to the directory with market data files.
 * @param marketDataFiles The vector with found market data files.
 */
void logMarketDataSearch(
        const std::string& dataPath, const std::vector<std::string>& marketDataFiles);
/**
 * @brief Logs that InputData is read into Manager.
 */
void logDoneReadInput();
/**
 * @brief Logs that a Manager with InputLinks is created.
 */
void logDoneWithLinks();
/**
 * @brief Logs that algorithm with name algoName is created.
 *
 * @param algoName The name of the created algorithm.
 */
void logCreatedAlgorithm(const std::string& algoName);

/**
 * @brief Logs that a certain spot strategy is ignored.
 */
void logIgnoreSpot(const std::string& spotName);
/**
 * @brief Logs that a certain allotment strategy is ignored.
 */
void logIgnoreLong(const std::string& longName);

