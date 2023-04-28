#include "validator.h"
#include "../logging/logging.h"
#include "../evaluation/evaluation_general.h"
#include "../algorithm/ialgorithm.h"
#include "../algorithm/strategic_algorithm.h"

namespace sea {

using std::vector;
using std::size_t;

bool checkSelection(const vector<bool>& selection, const vector<vector<ui32>>& groups) {
    for (const auto& member : groups) {
        size_t count = 0;
        for (auto item : member) {
            if (selection[item]) ++count;
        }
        if (count > 1) return false;
    }
    return true;
}

void Validator::bruteForceDo(size_t groupProductLong) {
    const auto& input = config.input->getConstData();
    vector<bool> haveAllotment(input.allotments.size(), false);
    auto groupsCopy = input.allotmentGroups;
    for (const auto& member : groupsCopy) {
        for (auto item : member) {
            haveAllotment[item] = true;
        }
    }
    for (ui32 idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        if (!haveAllotment[idAllotment]) {
            groupsCopy.push_back({idAllotment});
            assert(groupsCopy.back().size() == 1 && groupsCopy.back()[0] == idAllotment);
        }
    }
    haveAllotment.assign(input.allotments.size(), false);
        if (groupProductLong < (1u << input.allotments.size())) {
            vector<short> groupPlaces(groupsCopy.size(), 0);
            bool allZero = true;
            do {
                if (checkSelection(haveAllotment, groupsCopy)) {
                    processSelection(haveAllotment);
                }
                for (ui32 place = 0; place < groupPlaces.size(); ++place) {
                    groupPlaces[place] = groupPlaces[place] + 1;
                    if (groupPlaces[place] == int(1 + groupsCopy[place].size())) {
                        groupPlaces[place] = 0;
                    } else {
                        break;
                    }
                }
                haveAllotment.assign(input.allotments.size(), false);
                for (ui32 place = 0; place < groupPlaces.size(); ++place) {
                    auto target = groupPlaces[place];
                    if (target) {
                        auto id = groupsCopy[place][target - 1];
                        haveAllotment[id] = true;
                    }
                }
                allZero = true;
                for (auto item : groupPlaces) { allZero = (allZero && item); };
            } while (!allZero);
        } else {
            for (size_t mask = 0; mask < (1u << input.allotments.size()); ++mask) {
                haveAllotment.assign(input.allotments.size(), false);
                for (size_t place = 0; place < input.allotments.size(); ++place) {
                    if (mask & (1u << place)) {
                        haveAllotment[place] = true;
                    }
                }
                if (checkSelection(haveAllotment, groupsCopy)) {
                    processSelection(haveAllotment);
                }
            }
        }
}

void Validator::randomSearch() {
    vector<double> probas = {0.1, 0.3, 0.5, 0.8, 0.9};
    for (auto proba : probas) {
        size_t deep_iteration_count = 1024;
        size_t whole_iteration_limit = 1000000000;
        size_t query_count = 0;
        std::random_device rd;
        std::mt19937 gen(rd());

        std::bernoulli_distribution d(proba);

        const auto& input = config.input->getConstData();
        for (size_t iteration = 0;
                iteration < whole_iteration_limit && query_count < deep_iteration_count;
                ++iteration) {
            vector<bool> haveAllotment(input.allotments.size(), 0);
            for (ui32 idx = 0; idx < input.allotments.size(); ++idx) {
                haveAllotment[idx] = d(gen);
            }
            if (checkSelection(haveAllotment, input.allotmentGroups)) {
                processSelection(haveAllotment);
                ++query_count;
            }
        }
    }
}

void Validator::bendersCheck() {
    auto benders = std::make_shared<strategy::BendersLRAllotmentStrategy>(strategyConfigs.bendersAllotment);
    auto ipopt = std::make_shared<strategy::IpoptAllotmentStrategy>(strategyConfigs.ipoptAllotment);

    auto bendersDecision = benders->provideAllotments();
    auto ipoptDecision = ipopt->provideAllotments();

    auto bendersSelection = bendersDecision->getConstData().allotmentAccepted;
    auto ipoptSelection = ipoptDecision->getConstData().allotmentAccepted;

    auto& logger = logging::getValidationLogger();
    logger.debugStream() << "Benders VS Ipopt";
    logger.debugStream() << "Evaluating Benders proposal: ";
    processSelection(bendersSelection, true);
    logger.debugStream() << "Evaluating Ipopt proposal: ";
    processSelection(ipoptSelection, true);
}

void Validator::performChecks() {
    const auto& input = config.input->getConstData();

    std::size_t groupProductLong = 0;
    std::size_t tooMuch = 1025;

    bool bruteForce = (input.allotments.size() > 10);

    for (const auto& group : input.allotmentGroups) {
        groupProductLong *= (1 + group.size());
        if (groupProductLong > tooMuch) {
            bruteForce = false;
        }
    }

    if (bruteForce) {
        bruteForceDo(groupProductLong);
    } else {
        randomSearch();
    }
}

void Validator::processSelection(vector<bool> selection, bool processAnyway) {
    double ipoptScore = 0.;
    double lrScore = 0.;

    auto ipoptBackend = std::make_shared<backend::IpoptBackend>(backendConfigs.ipoptConfig);
    auto lrBackend =
        std::make_shared<backend::LagrangianRelaxationBackend> (backendConfigs.lrConfig);
    auto ipoptDecision = ipoptBackend->bendersQuery(selection, &ipoptScore);

    State state;
    initState(config.input->getConstData(), &state);

    auto lrDecision = ipoptDecision->deepCopy();
    auto duals = lrBackend->provideDuals(state, ipoptDecision->deepCopy(), nullptr, &lrScore);

    if (lrScore * ipoptScore < 0 || processAnyway ) {
        auto& logger = logging::getValidationLogger();
        if (lrScore * ipoptScore < 0) {
            logger.debugStream() << "Error detected: < 0.";
        }

        logger.infoStream() << "ipoptScore = " << ipoptScore;
        logger.infoStream() << "lrScore = " << lrScore;

        EvaluatorConfig evalConfig;
        evalConfig.inputManager = config.input;
        evalConfig.linksManager = config.links;
        evalConfig.needMemory = false;

        strategyConfigs.ipoptAllotment.forcedAllotments = selection;
        strategyConfigs.ipoptAllotment.useForcedAllotments = true;

        algo::StrategicAlgorithmConfig algoIpoptConfig;
        algoIpoptConfig.allotmentStrategy =
            std::make_shared<strategy::IpoptAllotmentStrategy>(strategyConfigs.ipoptAllotment);
        algoIpoptConfig.spotMarketStrategy =
            std::make_shared<strategy::IpoptSpotMarketStrategy>(strategyConfigs.ipoptSpot);

        algo::StrategicAlgorithmConfig algoLRConfig;
        algoLRConfig.allotmentStrategy =
            std::make_shared<strategy::IpoptAllotmentStrategy>(strategyConfigs.ipoptAllotment);
        algoLRConfig.spotMarketStrategy =
            std::make_shared<strategy::LRCuttingPlaneSpotMarketStrategy>(strategyConfigs.lrSpot);

        Evaluator evaluator(evalConfig);

        auto ipoptAlg = std::make_shared<algo::StrategicAlgorithm>(algoIpoptConfig);
        auto lrAlg = std::make_shared<algo::StrategicAlgorithm>(algoLRConfig);

        ipoptAlg->reset();
        lrAlg->reset();

        auto ipoptStats = evaluator.calc(ipoptAlg, config.marketSet[0]);
        auto lrStats = evaluator.calc(lrAlg, config.marketSet[0]);

        logger.infoStream() << "evaluated ipopt_spot: " << ipoptStats.fullProfit;
        logger.infoStream() << "evaluated lr_spot: " << lrStats.fullProfit;
    }
}


} // namespace sea
