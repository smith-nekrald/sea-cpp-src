// This file implements functions defined in start.h.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include <iostream>
#include <fstream>
#include <chrono>
#include <cassert>
#include <filesystem>
#include <regex>

#include <jsoncpp/json/json.h>

#include "start.h"
#include "common.h"
#include "manager.h"
#include "algorithm/strategic_algorithm.h"
#include "algorithm/greedy_algorithm.h"
#include "backend/backend_general.h"
#include "backend/lagrangian_relaxation/gm_sea/gm_src/gm_types.h"
#include "strategy/general_strategy.h"
#include "reader/reader_general.h"
#include "evaluation/evaluation_general.h"
#include "logging/logging.h"
#include "sense/validator.h"

namespace fs = std::filesystem;
using std::size_t;
using namespace gm;

void fillBackendConfig(const json::Value& configRoot, IpoptBackendConfig& config) {
    auto backendsConfig = configRoot["backends_config"];
    if (!backendsConfig.isMember("ipopt_backend_config")) {
        logNoIpoptBackendConfiguration();
        return;
    }
    auto ipoptRoot = backendsConfig["ipopt_backend_config"];
    auto solverType = ipoptRoot["solver_type"].asString();
    config.needMemory = configRoot["launch_config"]["need_memory"].asBool();
    config.defaultUtilizationRatio = ipoptRoot["default_utilization"].asDouble();
    if (config.needMemory) {
        config.saveVariables = false;
    }
    if (solverType == "mumps") {
        config.solver = sea::backend::LinearSolver::MUMPS;
    } else if (solverType == "ma27") {
        config.solver = sea::backend::LinearSolver::MA27;
    } else if (solverType == "ma97") {
        config.solver = sea::backend::LinearSolver::MA97;
    } else if (solverType == "ma86") {
        config.solver = sea::backend::LinearSolver::MA86;
    } else if (solverType == "ma57") {
        config.solver = sea::backend::LinearSolver::MA57;
    } else if (solverType == "ma77") {
        config.solver = sea::backend::LinearSolver::MA77;
    } else {
        throw std::logic_error("Unknown solver type.");
    }

    auto derivativeStrategy = ipoptRoot["derivative_strategy"].asString();
    if (derivativeStrategy == "forward") {
        config.derivativeComputation = sea::backend::DerivativeStrategy::FORWARD;
    } else if (derivativeStrategy == "backward") {
        config.derivativeComputation = sea::backend::DerivativeStrategy::BACKWARD;
    } else {
        throw std::logic_error("Unknown derivative strategy");
    }
    config.filePrintLevel = ipoptRoot["file_print_level"].asInt();
    config.printLevel = ipoptRoot["print_level"].asInt();

    auto memoryManagement = ipoptRoot["memory_management"];
    if (memoryManagement == "exact") {
        config.memoryManagement = sea::backend::MemoryStrategy::EXACT_COMPUTATION;
    } else if (memoryManagement == "bfgs" ) {
        config.memoryManagement = sea::backend::MemoryStrategy::LIMITED_MEMORY_BFGS;
    } else if (memoryManagement == "sr1" ) {
        config.memoryManagement = sea::backend::MemoryStrategy::LIMITED_MEMORY_SR1;
    } else {
        throw std::logic_error("Unknown memory management strategy");
    }

    config.needDescriptionsInIndex = ipoptRoot["write_descriptions"].asBool();
    config.path_prefix = ipoptRoot["path_prefix"].asString();
}

void fillBackendConfig(const json::Value& configRoot, LagrangianRelaxationBackendConfig& config) {
    auto backendsConfig = configRoot["backends_config"];
    if (!backendsConfig.isMember("lr_backend_config")) {
        logNoLagrangianBackendConfiguration();
        return;
    }
    auto lrRoot = backendsConfig["lr_backend_config"];

    // Common entries set.
    config.needMemory = configRoot["launch_config"]["need_memory"].asBool();

    // Initialization and bounds.
    config.maxCapacityCost = lrRoot["max_capacity_cost"].asDouble();
    config.initCapacityCost = lrRoot["init_capacity_cost"].asDouble();

    // Randomization.
    auto initStrategy = lrRoot["init_strategy"].asString();
    if (initStrategy == "both") {
        config.initStrategy = sea::backend::RandomInitType::RANDOM_BOTH;
    } else if (initStrategy == "uniform") {
        config.initStrategy = sea::backend::RandomInitType::RANDOM_UNIFORM;
    } else if (initStrategy == "normal") {
        config.initStrategy = sea::backend::RandomInitType::RANDOM_NORMAL;
    } else {
        throw std::logic_error("Unknown LR init strategy.");
    }
    config.normalStd =  lrRoot["normal_std"].asDouble();
    config.normalMean = lrRoot["normal_mean"].asDouble();
    config.uniformMin = lrRoot["uniform_left"].asDouble();
    config.uniformMax = lrRoot["uniform_right"].asDouble();
    config.bernoullyProba = lrRoot["bernoully_proba"].asDouble();
    config.seed = lrRoot["seed"].asInt();

    // Debug Information.
    config.clpLogLevel = lrRoot["clp_log_level"].asInt();
    config.keepStory = lrRoot["write_iter_story"].asBool();

    // Common optimization method parameters.
    auto optimizationAlgo = lrRoot["optimization_algo"].asString();
    if (optimizationAlgo == "cutting_plane") {
        config.optimizationAlgo = sea::backend::OptimizationAlgo::CUTTING_PLANE;
    } else if (optimizationAlgo == "gm") {
        config.optimizationAlgo = sea::backend::OptimizationAlgo::GM;
    } else {
        throw std::logic_error("Unknown LR optimization algorithm!");
    }
    config.maxSubgradientIterations = lrRoot["max_subgradient_iterations"].asInt();
    config.subgradientPrecision = lrRoot["subgradient_precisioin"].asDouble();

    // Common backend parameters.
    auto cutoffDecision = lrRoot["cutoff_decision"];
    if (cutoffDecision == "itinerary") {
        config.cutoffDecisionType = sea::backend::CutoffDecisionType::BY_ITINERARY;
    } else if (cutoffDecision == "event") {
        config.cutoffDecisionType = sea::backend::CutoffDecisionType::BY_EVENT;
    } else {
        throw std::logic_error("Unknown cutoff decision.");
    }
    auto clpSolution = lrRoot["clp_solution"];
    if (clpSolution == "primal") {
        config.clpMethod = sea::backend::ClpSolutionConfig::CLP_PRIMAL;
    } else if (clpSolution == "dual") {
        config.clpMethod = sea::backend::ClpSolutionConfig::CLP_DUAL;
    } else {
        throw std::logic_error("Unknown Clp solution strategy.");
    }
    config.globalPrecision = lrRoot["global_precision"].asDouble();
    config.zeroIgnorePrecision = lrRoot["zero_ignore_precision"].asDouble();
    config.coeffReg = lrRoot["reg_coeff"].asDouble();

    // Cutting-plane related configurations and tweaks.
    if (config.optimizationAlgo == sea::backend::OptimizationAlgo::CUTTING_PLANE) {
        config.minSubgradientIterations = lrRoot["min_subgradient_iterations"].asUInt();
        config.initPointCount = lrRoot["init_point_count"].asUInt();
        config.deque_size = lrRoot["deque_size"].asUInt();
        config.useEquationsRestart = lrRoot["use_reset"].asBool();
        config.singleRestart  = lrRoot["single_restart"].asBool();
        config.restartPeriod =  lrRoot["reset_period"].asUInt();
        config.immortalLimit = lrRoot["immortal_limit"].asUInt();
        config.immortalShuffleCount = lrRoot["immortal_shuffle_count"].asUInt();
    }

    // UFGM-related configurations and tweaks.
    if (config.optimizationAlgo == sea::backend::OptimizationAlgo::GM) {
        auto& gmParams = lrRoot["gm_params"];
        config.L0 = gmParams["L0"].asDouble();
        assert(config.coeffReg.value() > 0);
        auto ufgmProxType = gmParams["gm_prox_type"];
        if (ufgmProxType == "l2squared") {
            config.ufgmProxType = GmProxType::L2SQUARED;
        } else {
            throw std::logic_error("Unknown gm prox type!");
        }
        auto ufgmRegType = gmParams["gm_reg_type"];
        if (ufgmRegType == "l2squared") {
            config.ufgmRegType =GmRegularizerType::L2SQUARED;
        } else {
            throw std::logic_error("Unknown reg type!");
        }
        auto algoName = gmParams["algo_inside"];
        if (algoName == "ufgm") {
            config.gradientOptimizer = GradientFamily::UFGM;
        } else if (algoName == "upgm") {
            config.gradientOptimizer = GradientFamily::UPGM;
        } else if (algoName == "gm") {
            config.gradientOptimizer = GradientFamily::GM;
        }
    }
}

void fillBackendConfig(const json::Value& configRoot, BendersAllotmentBackendConfig& config) {
    auto backendsConfig = configRoot["backends_config"];
    if (!backendsConfig.isMember("benders_backend_config")) {
        logNoBendersBackendConfiguration();
        return;
    }
    auto bendersRoot = backendsConfig["benders_backend_config"];
    config.needMemory = configRoot["launch_config"]["need_memory"].asBool();
    config.cbcFileLogLevel = bendersRoot["file_log_level"].asInt();
    config.cbcLogLevel = config.cbcFileLogLevel;
}

void fillBackendConfig(const json::Value& configRoot, DetCutPlaneBackendConfig& config) {
    auto backendsConfig = configRoot["backends_config"];
    if (!backendsConfig.isMember("det_cut_plane_backend_config")) {
        logNoDCPBackendConfiguration();
        return;
    }
    auto dcpRoot = backendsConfig["det_cut_plane_backend_config"];
    config.needMemory = configRoot["launch_config"]["need_memory"].asBool();
    config.cbcFileLogLevel = dcpRoot["file_log_level"].asInt();
    config.cbcLogLevel = config.cbcFileLogLevel;
    config.defaultUtilizationRatio = dcpRoot["default_utilization"].asDouble();
    config.initialPlanes = dcpRoot["initial_planes_count"].asInt();
    config.integerTolerance = dcpRoot["integer_tolerance"].asDouble();
    config.iterations = dcpRoot["max_iterations"].asInt();
    config.needError = dcpRoot["need_error"].asDouble();
    config.seed = dcpRoot["seed"].asInt();
    config.stopOnInfeasible = dcpRoot["stop_on_infeasible"].asBool();
    config.tolerateConstraints = dcpRoot["tolerate_constraints"].asBool();
}

std::vector<std::string> getMarketDataFiles(std::string dataPath, std::size_t count_limit) {
    std::vector<std::string> market_data_files;
    for (auto& entry: fs::directory_iterator(dataPath)) {
        if (!fs::is_regular_file(entry)) {
            continue;
        }
        std::string filename = fs::path(entry).filename().string();
        std::string filepath = fs::path(entry).string();
        std::smatch what;
        if (std::regex_match(filename, what, std::regex("market.*"))) {
            market_data_files.push_back(filepath);
        }
    }
    std::sort(market_data_files.begin(), market_data_files.end());
    if (market_data_files.size() >= count_limit) {
        market_data_files.resize(count_limit);
    }
    return market_data_files;
}

sea::ConstInputManagerPtr getInput(std::string dataPath, bool needMemory) {
    sea::ManagerConfig config = {needMemory, dataPath, false} ;
    sea::InputManagerPtr input = std::make_shared<sea::DataManager<sea::InputData>>(config);
    return input;
}

sea::ConstLinksManagerPtr getLinks(
        const sea::ConstInputManagerPtr& inputManager, bool needMemory) {
    std::string filePath = "links_" + sea::makeUniqueFileName() + ".data";
    sea::ManagerConfig config = {needMemory, filePath, true} ;
    sea::LinksManagerPtr links = std::make_shared<sea::DataManager<sea::InputLinks>>(config);
    sea::createInputLinks(inputManager->getConstData(), links->get());
    return links;
}

std::vector<sea::ConstMarketManagerPtr> getMarketVector(
        const std::vector<std::string>& pathVector, bool needMemory) {
    std::vector<sea::ConstMarketManagerPtr> marketVector;
    for (const auto& dataPath : pathVector) {
        sea::ManagerConfig config = {needMemory, dataPath, false};
        sea::MarketManagerPtr market = std::make_shared<
            sea::DataManager<sea::MarketData>>(config);
        market->release();
        marketVector.push_back(market);
        logFinishedReadingMarket(dataPath);
    }
    return marketVector;
}

void prepareBackendConfigHolder(const json::Value& configRoot,
        sea::ConstInputManagerPtr inputManager, sea::ConstLinksManagerPtr linksManager,
        sea::BackendConfigHolder* holderPtr) {
    auto launchConfig = configRoot["launch_config"];
    bool debugMode = (launchConfig["launch_mode"].asString() == "debug");
    IpoptBackendConfig ipoptBackendConfig;
    ipoptBackendConfig.inputManager = inputManager;
    ipoptBackendConfig.linksManager = linksManager;
    fillBackendConfig(configRoot, ipoptBackendConfig);
    if (debugMode) {
        ipoptBackendConfig.needDescriptionsInIndex = true;
    }

    LagrangianRelaxationBackendConfig lrBackendConfig;
    lrBackendConfig.inputManager = inputManager;
    lrBackendConfig.linksManager = linksManager;
    fillBackendConfig(configRoot, lrBackendConfig);

    BendersAllotmentBackendConfig bendersBackendConfig;
    bendersBackendConfig.inputManager = inputManager;
    bendersBackendConfig.linksManager = linksManager;
    fillBackendConfig(configRoot, lrBackendConfig);

    DetCutPlaneBackendConfig dcpBackendConfig;
    dcpBackendConfig.inputManager = inputManager;
    dcpBackendConfig.linksManager = linksManager;
    fillBackendConfig(configRoot, dcpBackendConfig);

    sea::BackendConfigHolder& holder = *holderPtr;
    holder.bendersConfig = bendersBackendConfig;
    holder.ipoptConfig = ipoptBackendConfig;
    holder.lrConfig = lrBackendConfig;
    holder.dcpConfig = dcpBackendConfig;
}

void prepareSpotStrategies(
        const json::Value& configRoot, const sea::BackendConfigHolder& holder,
        sea::ConstInputManagerPtr inputManager, sea::ConstLinksManagerPtr linksManager,
        sea::StrategyConfigs* strategyConfigsPtr,
        std::map<std::string, sea::strategy::ISpotMarketStrategyPtr>* spotStrategiesPtr,
        std::vector<std::string>* spotNamesPtr) {

    sea::StrategyConfigs& strategyConfigs = *strategyConfigsPtr;
    std::vector<std::string> turnOffSpot;
    std::map<std::string, sea::strategy::ISpotMarketStrategyPtr>&
        spotStrategies = *spotStrategiesPtr;
    std::vector<std::string>& spotNames = *spotNamesPtr;

    auto launchConfig = configRoot["launch_config"];
    auto strategyConfig = configRoot["strategy_config"];
    auto spotStrategyConfig = strategyConfig["spot_strategy_config"];

    sea::strategy::SpotMarketStrategyConfig commonSpotConfig;
    commonSpotConfig.moveUpdateInterval = spotStrategyConfig["move_update_interval"].asBool();
    commonSpotConfig.needWarmBackends = spotStrategyConfig["warm_backends"].asBool();
    commonSpotConfig.keepStory = spotStrategyConfig["keep_story"].asBool();

    commonSpotConfig.inputManager = inputManager;
    commonSpotConfig.linksManager = linksManager;
    commonSpotConfig.updateInterval = spotStrategyConfig["update_interval"].asDouble();


    for (std::size_t idx = 0; idx < launchConfig["turn_off_spot"].size(); ++idx) {
        auto name = launchConfig["turn_off_spot"][static_cast<int>(idx)];
        turnOffSpot.push_back(name.asString());
    }

    for (std::size_t idx = 0; idx < launchConfig["spot_strategies"].size(); ++idx) {
        auto name = launchConfig["spot_strategies"][static_cast<int>(idx)];
        auto spotName = name.asString();
        if (std::find(std::begin(turnOffSpot),
                    std::end(turnOffSpot), spotName) != std::end(turnOffSpot)) {
            logIgnoreSpot(spotName);
            continue;
        }
        if (spotName == "lr" || spotName == "nospot_lr") {
            sea::strategy::LRBasedSpotMarketStrategyConfig config;
            config.backendConfigs = holder;
            config.abstractConfig = commonSpotConfig;
            config.abstractConfig.needWarmBackends = true;
            config.abstractConfig.type = sea::SpotStrategyType::LR;
            auto lrStrategyConfig = strategyConfig["spot_strategy_config"]["lr_strategy_config"];

            config.abstractConfig.utilizationRatio = lrStrategyConfig[
                "hard_utilization_ratio"].asDouble();
            config.doHardUpdate = lrStrategyConfig["do_hard_update"].asBool();
            config.softUpdatePeriod = lrStrategyConfig["soft_update_interval"].asDouble();
            strategyConfigs.lrSpot = config;

            config.askIpoptDuals = lrStrategyConfig["ask_ipopt_duals"].asBool();

            if (spotName == "lr") {
                spotStrategies[spotName] = std::make_shared<
                    sea::strategy::LRBasedSpotMarketStrategy>(config);
            } else if (spotName == "nospot_lr")  {
                spotStrategies[spotName] = std::make_shared<sea::strategy::LTSpotMarketStrategy<
                    sea::strategy::LRBasedSpotMarketStrategy>>(config);
            }
        } else if (spotName == "ipopt" || spotName == "nospot_ipopt") {
            sea::strategy::IpoptSpotMarketStrategyConfig config;
            config.backendConfigs = holder;
            config.abstractConfig = commonSpotConfig;
            config.abstractConfig.needWarmBackends = true;
            config.abstractConfig.type = sea::SpotStrategyType::IPOPT;
            strategyConfigs.ipoptSpot = config;
            auto ipoptSpotConfig = strategyConfig["spot_strategy_config"]["ipopt_strategy_config"];
            config.abstractConfig.utilizationRatio = ipoptSpotConfig[
                "hard_utilization_ratio"].asDouble();
            if (spotName == "ipopt") {
                spotStrategies[spotName] = std::make_shared<
                    sea::strategy::IpoptSpotMarketStrategy>(config);
            } else if (spotName == "nospot_ipopt") {
                spotStrategies[spotName] = std::make_shared<sea::strategy::LTSpotMarketStrategy<
                    sea::strategy::IpoptSpotMarketStrategy>>(config);
            }
        } else if (spotName == "hybrid") {
            sea::strategy::HybridSpotMarketStrategyConfig config;
            config.backendConfigs = holder;
            config.abstractConfig = commonSpotConfig;
            config.abstractConfig.type =
                sea::SpotStrategyType::HYBRID;
            config.abstractConfig.needWarmBackends = true;
            auto hybridSpotConfig = strategyConfig[
                "spot_strategy_config"]["hybrid_strategy_config"];
            config.abstractConfig.utilizationRatio = hybridSpotConfig[
                "hard_utilization_ratio"].asDouble();
            spotStrategies[spotName] = std::make_shared<
                sea::strategy::HybridSpotMarketStrategy>(config);
        } else {
            throw std::logic_error("Unknown spot strategy name!");
        }
        spotNames.push_back(spotName);
    }
    logCreatedSpotMarketStrategies();
}

void prepareAllotmentStrategies(const json::Value& configRoot,
        const sea::BackendConfigHolder& holder,
        sea::ConstInputManagerPtr inputManager,
        sea::ConstLinksManagerPtr linksManager,
        sea::StrategyConfigs* strategyConfigsPtr,
        std::map<std::string, sea::strategy::IAllotmentStrategyPtr>* allotmentStrategiesPtr,
        std::vector<std::string>* allotmentNamesPtr) {

    sea::StrategyConfigs& strategyConfigs = *strategyConfigsPtr;
    std::map<std::string, sea::strategy::IAllotmentStrategyPtr>&
        allotmentStrategies = *allotmentStrategiesPtr;
    std::vector<std::string>& allotmentNames = *allotmentNamesPtr;

    auto launchConfig = configRoot["launch_config"];
    auto strategyConfig = configRoot["strategy_config"];
    auto allotmentStrategyConfig = strategyConfig["allotment_strategy_config"];

    sea::strategy::AllotmentStrategyConfig commonLongConfig;
    commonLongConfig.storeInitialDecision =
        allotmentStrategyConfig["store_initial_decision"].asBool();
    commonLongConfig.linksManager = linksManager;
    commonLongConfig.inputManager = inputManager;

    std::vector<std::string> turnOffLong;
    for (std::size_t idx = 0; idx < launchConfig["turn_off_allotment"].size(); ++idx) {
        auto name = launchConfig["turn_off_allotment"][static_cast<int>(idx)];
        turnOffLong.push_back(name.asString());
    }

    for (std::size_t idx = 0; idx < launchConfig["allotment_strategies"].size(); ++idx) {
        auto name = launchConfig["allotment_strategies"][static_cast<int>(idx)];
        auto longName = name.asString();
        if (std::find(std::begin(turnOffLong), std::end(turnOffLong),
                    longName) != std::end(turnOffLong)) {
            logIgnoreLong(longName);
            continue;
        }
        if (longName == "ipopt" || longName == "ipopt_nospot_aware") {
            sea::strategy::IpoptAllotmentStrategyConfig config;
            auto ipoptJsonConfig = allotmentStrategyConfig["ipopt_strategy_config"];
            config.abstractConfig = commonLongConfig;

            config.backendConfigs = holder;
            config.abstractConfig.type = sea::AllotmentStrategyType::IPOPT;
            config.abstractConfig.defaultUtilizationRatio = ipoptJsonConfig[
                "default_utilization_ratio"].asDouble();

            if (longName == "ipopt") {
                allotmentStrategies[longName] = std::make_shared<
                    sea::strategy::IpoptAllotmentStrategy>(config);
            } else if (longName == "ipopt_nospot_aware") {
                allotmentStrategies[longName] = std::make_shared<
                    sea::strategy::NospotAwareAllotmentStrategy<
                    sea::strategy::IpoptAllotmentStrategy>>(config);
            }
            strategyConfigs.ipoptAllotment = config;
        } else if (longName == "benders") {
            sea::strategy::BendersLRAllotmentStrategyConfig config;
            config.abstractConfig = commonLongConfig;

            auto bendersJsonConfig = allotmentStrategyConfig["benders_strategy_config"];
            config.config.iterationCount = bendersJsonConfig["max_iteration_count"].asInt();
            config.config.objectiveBlending = bendersJsonConfig["objective_blending"].asDouble();
            config.config.bendersStopPrecision = bendersJsonConfig["precision"].asDouble();
            config.abstractConfig.defaultUtilizationRatio = bendersJsonConfig[
                    "default_utilization_ratio"].asDouble();
            config.backendConfigs = holder;
            config.abstractConfig.type = sea::AllotmentStrategyType::BENDERS;
            allotmentStrategies[longName] = std::make_shared<
                sea::strategy::BendersLRAllotmentStrategy>(config);
            strategyConfigs.bendersAllotment = config;
        } else if (longName == "det_cut_plane") {
            sea::strategy::DetCutPlaneAllotmentStrategyConfig config;
            config.abstractConfig = commonLongConfig;

            auto dcpJsonConfig = allotmentStrategyConfig["det_cut_plane_strategy_config"];
            config.backendConfigs = holder;
            config.abstractConfig.type = sea::AllotmentStrategyType::DET_CUT_PLANE;
            config.abstractConfig.defaultUtilizationRatio = dcpJsonConfig[
                "default_utilization_ratio"].asDouble();
            allotmentStrategies[longName] = std::make_shared<
                sea::strategy::DetCutPlaneAllotmentStrategy>(config);
        } else if (longName == "zero") {
            sea::strategy::ZeroAllotmentStrategyConfig config;
            config.abstractConfig = commonLongConfig;

            config.abstractConfig.defaultUtilizationRatio = 1.0;
            config.backendConfigs = holder;
            config.abstractConfig.type = sea::AllotmentStrategyType::ZERO_IPOPT;
            allotmentStrategies[longName] = std::make_shared<
                sea::strategy::ZeroAllotmentStrategy>(config);
        } else if (longName == "null") {
            sea::strategy::NullAllotmentStrategyConfig config;
            config.abstractConfig = commonLongConfig;

            config.abstractConfig.defaultUtilizationRatio = 1.0;
            config.backendConfigs = holder;
            config.abstractConfig.type = sea::AllotmentStrategyType::ZERO_NULL;
            allotmentStrategies[longName] = std::make_shared<
                sea::strategy::NullAllotmentStrategy>(config);
        } else {
            throw std::logic_error("Unknown allotment strategy name! - " + longName);
        }
        allotmentNames.push_back(longName);
    }
    logCreatedAllotmentStrategies();
}

void prepareConfigurationPairs(const json::Value& configRoot,
        const std::vector<std::string>& spotNames,
        const std::vector<std::string>& allotmentNames,
        std::vector<std::pair<std::string, std::string>>* configurationPairsPtr) {
    auto launchConfig = configRoot["launch_config"];
    std::vector<std::pair<std::string, std::string>>&
            configurationPairs = * configurationPairsPtr;
    std::vector<std::pair<std::string, std::string>> ignoredPairs;

    for (std::size_t idx = 0; idx < launchConfig["ignore_pairs"].size(); ++idx) {
        auto pairData = launchConfig["ignore_pairs"][static_cast<int>(idx)];
        auto spotValue = pairData["spot"].asString();
        auto longValue = pairData["allotment"].asString();
        ignoredPairs.push_back({spotValue, longValue});
    }
    for (const auto& lhs : spotNames) {
        for (const auto& rhs : allotmentNames) {
            auto join = std::make_pair(lhs, rhs);
            bool found = false;
            for (const auto& cand : ignoredPairs) {
                if (cand == join) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                configurationPairs.push_back(join);
            }
        }
    }
}

void startValidation(const std::vector<std::string>& marketDataFiles, bool needMemory,
    sea::ConstInputManagerPtr inputManager, sea::ConstLinksManagerPtr linksManager,
    const sea::BackendConfigHolder& holder, const sea::StrategyConfigs& strategyConfigs) {
    auto marketVector = getMarketVector(marketDataFiles, needMemory);

    sea::ValidatorConfig validatorConfig;
    validatorConfig.input = inputManager;
    validatorConfig.links = linksManager;
    validatorConfig.marketSet = marketVector;

    sea::Validator validator(validatorConfig, holder, strategyConfigs);
    validator.bendersCheck();
    validator.performChecks();
    exit(0);
}

void startAlgorithms(const json::Value& configRoot,
        const std::vector<std::string> marketDataFiles, bool needMemory,
        sea::ConstInputManagerPtr inputManager, sea::ConstLinksManagerPtr linksManager,
        const std::vector<sea::algo::IAlgorithmPtr>& algoVector) {
    sea::LauncherConfig launcherConfig;
    bool writeEvalStory = configRoot["write_eval_story"].asBool();
    launcherConfig.writeStory = writeEvalStory;
    launcherConfig.linksManager = linksManager;
    launcherConfig.inputManager = inputManager;
    launcherConfig.needMemory = needMemory;
    launcherConfig.algorithms = algoVector;
    launcherConfig.marketDataSetPaths = marketDataFiles;

    logReadyToLaunch();
    sea::Launcher launcher(launcherConfig);
    auto launchResult = launcher.doLaunches();
    logLaunchFinished();

    logReadyToAnalyze();
    sea::Analyzer analyzer;
    auto launchConfig = configRoot["launch_config"];
    auto preciseFilePath = launchConfig["long_output_file"].asString();
    auto generalFilePath = launchConfig["short_output_file"].asString();
    std::ofstream preciseOut(preciseFilePath), generalOut(generalFilePath);
    analyzer.doAnalysis(launchResult, preciseOut, generalOut);
    logAnalysisFinished();
}

void start(const json::Value& configRoot) {
    auto pathToLoggingConfig = configRoot["logging_config"]["path_to_config"].asString();
    logging::configureLogger(pathToLoggingConfig);
    logStartedWithConfig(pathToLoggingConfig);

    auto launchConfig = configRoot["launch_config"];
    bool needMemory = launchConfig["need_memory"].asBool();
    std::string dataPath = launchConfig["dataset_path"].asString();


    size_t marketLimit = launchConfig["market_dataset_limit"].asInt();
    auto marketDataFiles = getMarketDataFiles(dataPath, marketLimit);
    if (marketDataFiles.size() > marketLimit) {
        marketDataFiles.resize(marketLimit);
    }
    logMarketDataSearch(dataPath, marketDataFiles);

    sea::ConstInputManagerPtr inputManager = getInput(dataPath + "/input.txt", needMemory);
    logDoneReadInput();

    sea::ConstLinksManagerPtr linksManager = getLinks(inputManager, needMemory);
    logDoneWithLinks();

    // Prepare holder for backend confgs.
    sea::BackendConfigHolder holder;
    prepareBackendConfigHolder(configRoot, inputManager, linksManager, &holder);

    // Prepare spot strategies.
    sea::StrategyConfigs strategyConfigs;
    std::map<std::string, sea::strategy::ISpotMarketStrategyPtr> spotStrategies;
    std::vector<std::string> spotNames;
    prepareSpotStrategies(configRoot, holder, inputManager, linksManager,
            &strategyConfigs, &spotStrategies, &spotNames);

    // Prepare allotment strategies.
    std::map<std::string, sea::strategy::IAllotmentStrategyPtr> allotmentStrategies;
    std::vector<std::string> allotmentNames;
    prepareAllotmentStrategies(configRoot, holder, inputManager, linksManager,
            &strategyConfigs, &allotmentStrategies, &allotmentNames);

    // Prepare configuration pairs.
    std::vector<std::pair<std::string, std::string>> configurationPairs;
    prepareConfigurationPairs(configRoot, spotNames, allotmentNames, &configurationPairs);

    // Build algorithms.
    std::vector<sea::algo::IAlgorithmPtr> algoVector;
    // Start with Greedy algorithm (if relevant).
    const auto& greedyConfig = configRoot["greedy_config"];
    bool useGreedy = greedyConfig["use_greedy"].asBool();
    if (useGreedy) {
        sea::algo::GreedyConfig config;
        config.inputManager = inputManager;
        config.linksManager = linksManager;
        config.memoryOptimization = greedyConfig["memory_optimization"].asBool();
        config.trackStory = greedyConfig["track_history"].asBool();
        config.ignoreSpotMarket = greedyConfig["ignore_spot"].asBool();
        config.ignoreLongMarket = greedyConfig["ignore_long"].asBool();
        auto greedyAlgo = std::make_shared<sea::algo::GreedyAlgorithm>(config);
        algoVector.push_back(greedyAlgo);
        logCreatedAlgorithm(greedyAlgo->getName());
    }
    // Then build strategic algorithms.
    for (const auto& conf : configurationPairs) {
        sea::algo::StrategicAlgorithmConfig config;
        config.allotmentStrategy = allotmentStrategies[conf.second];
        config.spotMarketStrategy = spotStrategies[conf.first];
        auto algo = std::make_shared<sea::algo::StrategicAlgorithm>(config);
        algoVector.push_back(algo);
        logCreatedAlgorithm(algo->getName());
    }
    // Validation launch, if relevant.
    bool doValidation = configRoot["do_validation"].asBool();
    if (doValidation) {
        startValidation(marketDataFiles, needMemory, inputManager, linksManager,
                holder, strategyConfigs);
    // Otherwise, launch algorithms and analyze the results.
    } else {
        startAlgorithms(configRoot, marketDataFiles, needMemory,
                inputManager, linksManager, algoVector);
    }
}

