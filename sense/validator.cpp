// Implements methods and functions defined in validator.h.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "validator.h"
#include "../logging/logging.h"
#include "../evaluation/evaluation_general.h"
#include "../algorithm/ialgorithm.h"
#include "../algorithm/strategic_algorithm.h"

namespace sea {

using std::vector;
using std::size_t;

bool checkSelection(const vector<bool>& selection, const vector<vector<unsigned>>& groups) {
    for (const auto& member : groups) {
        size_t count = 0;
        for (auto item : member) {
            if (selection[item]) ++count;
        }
        if (count > 1) {
            return false;
        }
    }
    return true;
}

void Validator::bruteForceDo(size_t groupProductLong) {
    const auto& input = config.input->getConstData();
    vector<bool> haveAllotment(input.allotments.size(), false);
    auto groupsCopy = input.allotmentGroups;
    // Preparatory block. For those allotments not in allotmentGroups, create new
    // groups of one element and append such groups to the groups vector.
    for (const auto& member : groupsCopy) {
        for (auto item : member) {
            haveAllotment[item] = true;
        }
    }
    for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        if (!haveAllotment[idAllotment]) {
            groupsCopy.push_back({idAllotment});
            assert(groupsCopy.back().size() == 1 && groupsCopy.back()[0] == idAllotment);
        }
    }
    // Now iterate through all possible subsets of allotments selected.
    // Two ways to iterate through all possible selections. Whatever is faster.
    haveAllotment.assign(input.allotments.size(), false);
    if (groupProductLong < (1u << input.allotments.size())) {
        // From each group at most one allotment is selected. In total, the number of selections
        // is at most the product of group sizes
        vector<unsigned> groupPlaces(groupsCopy.size(), 0);
        bool allZero = true;
        do {
            if (checkSelection(haveAllotment, groupsCopy)) {
                processSelection(haveAllotment);
            }
            for (unsigned place = 0; place < groupPlaces.size(); ++place) {
                groupPlaces[place] = groupPlaces[place] + 1;
                if (groupPlaces[place] == groupsCopy[place].size()) {
                    groupPlaces[place] = 0;
                } else {
                    break;
                }
            }
            haveAllotment.assign(input.allotments.size(), false);
            for (unsigned place = 0; place < groupPlaces.size(); ++place) {
                auto target = groupPlaces[place];
                if (target) {
                    auto id = groupsCopy[place][target - 1];
                    haveAllotment[id] = true;
                }
            }
            allZero = true;
            for (auto item : groupPlaces) {
                allZero = (allZero && item);
            };
        } while (!allZero);
    } else {
        // Mask-based search. For each mask, infer selection and check afterwards.
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
    const auto& input = config.input->getConstData();
    // Iterate through bernoulli distribution probabilities.
    vector<double> probas = {0.1, 0.3, 0.5, 0.8, 0.9};
    for (auto proba : probas) {
        std::random_device randomDevice;
        std::mt19937 generator(randomDevice());
        std::bernoulli_distribution distribution(proba);
        // Now for each iteration generate allotments with distribution and process selection.
        size_t deep_iteration_count = 1024;
        size_t whole_iteration_limit = 1000000000;
        size_t request_count = 0;
        for (size_t iteration = 0; iteration < whole_iteration_limit
                && request_count < deep_iteration_count; ++iteration) {
            vector<bool> haveAllotment(input.allotments.size(), false);
            for (unsigned idx = 0; idx < input.allotments.size(); ++idx) {
                haveAllotment[idx] = distribution(generator);
            }
            if (checkSelection(haveAllotment, input.allotmentGroups)) {
                processSelection(haveAllotment);
                ++request_count;
            }
        }
    }
}


void Validator::bendersCheck() {
    auto benders = std::make_shared<strategy::BendersLRAllotmentStrategy>(
            strategyConfigs.bendersAllotment);
    auto ipopt = std::make_shared<strategy::IpoptAllotmentStrategy>(
            strategyConfigs.ipoptAllotment);

    auto bendersDecision = benders->provideAllotments();
    auto ipoptDecision = ipopt->provideAllotments();

    auto bendersSelection = bendersDecision->getConstData().allotmentAccepted;
    auto ipoptSelection = ipoptDecision->getConstData().allotmentAccepted;

    logEvaluatingBendersInBendersCheck();
    processSelection(bendersSelection, true);
    logEvaluatingIpoptInBendersCheck();
    processSelection(ipoptSelection, true);
}

void Validator::performChecks() {
    const auto& input = config.input->getConstData();

    std::size_t groupProductLong = 0;
    std::size_t tooMuchForBruteForce = 1025;

    bool bruteForce = (input.allotments.size() > 10);

    for (const auto& group : input.allotmentGroups) {
        groupProductLong *= (1 + group.size());
        if (groupProductLong > tooMuchForBruteForce) {
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
    double ipoptScore = 0.; // Score predicted by Ipopt fluid approximation.
    double lrScore = 0.; // Score predicted by Lagrangian relaxation.

    auto ipoptBackend = std::make_shared<backend::IpoptBackend>(backendConfigs.ipoptConfig);
    auto lrBackend =
        std::make_shared<backend::LagrangianRelaxationBackend> (backendConfigs.lrConfig);
    auto ipoptDecision = ipoptBackend->bendersQuery(selection, &ipoptScore);

    State state;
    initState(config.input->getConstData(), &state);

    auto lrDecision = ipoptDecision->deepCopy();
    auto duals = lrBackend->provideDuals(state, ipoptDecision->deepCopy(), nullptr, &lrScore);

    // If LR and Ipopt predict different signs, that sounds strange.
    // Therefore, a detailed study is required.
    if (lrScore * ipoptScore < 0 || processAnyway) {
        // Let us compute the evaluation score on some
        // market data trajectory for both LR and Ipopt.
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

        // Now log and compare predicted and evaluated scores.
        logScoresInProcessSelection(
                ipoptScore, lrScore, ipoptStats.fullProfit, lrStats.fullProfit);
    }
}


} // namespace sea
