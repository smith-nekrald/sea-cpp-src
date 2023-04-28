#include "launcher.h"
#include "estimator.h"
#include "../logging/logging.h"

namespace sea {

using std::map;
using std::vector;
using std::string;
using std::size_t;

Launcher::Launcher(const LauncherConfig& aConfig)
    : config(aConfig) {
    logging::getEvaluationLogger().debug("Initizalized Launcher.");
}

map<string, vector<Statistics>> Launcher::doLaunches() {
    map<string, vector<Statistics>> launchResults;
    EvaluatorConfig evaluatorConfig = {
        config.inputManager,
        config.linksManager,
        config.needMemory,
        config.writeStory
    };
    for (size_t idx = 0; idx < config.marketDataSetPaths.size();
            ++idx) {
        std::string dataPath = config.marketDataSetPaths[idx];
        sea::ManagerConfig aConfig =
            {config.needMemory.value(), dataPath, false};
        sea::MarketManagerPtr market =
            std::make_shared<sea::DataManager<
                sea::MarketData>>(aConfig);
        logging::getRootLogger().info("Finished reading market for dataPath: " + dataPath);

        auto& item = market;
        item->load();
        SmartEstimator estimator;
        auto estimation = estimator.calcUpperBound(
                config.inputManager->getConstData(),
                config.linksManager->getConstData(),
                item->getConstData());
        for (const auto& algo : config.algorithms) {

            logging::getEvaluationLogger().info("Started evaluation for algorithm: " + algo->getName());
            logging::getEvaluationLogger().info("Evaluation set index is: " + std::to_string(idx));

            algo->reset();
            algo->synchronizeStrategies();

            Evaluator evaluator(evaluatorConfig);

            logging::getOutTestLogger().debugStream() << "Evaluation Started: " << algo->getName() << " " << " market_idx= " << idx;

            auto stats = evaluator.calc(algo, item);
            if (config.writeStory) {
                auto algoStory = evaluator.getAlgoStory();
                auto evalStory = evaluator.getEvalStory();
                std::string filePath = "eval_story/story_" + makeUniqueFileName() + ".out";
                std::ofstream out(filePath);
                out << "Market: " << dataPath << std::endl;
                out << "Algo: " << algo->getName() << std::endl;
                out << "AlgoStory: " << std::endl;
                printStory(algoStory, out);
                out << "EvalStory: " << std::endl;
                printStory(evalStory, out);
                out.flush();
                out.close();
            }

            logging::getOutTestLogger().debugStream() << "Evaluation Finished.";

            stats.estimation = estimation;

            assert(stats.allotmentProfit <= estimation.allotment);
            assert(stats.spotProfit <= estimation.spotMarket);

            assert(stats.spotProfit + stats.allotmentProfit <= estimation.allotment + estimation.spotMarket);

            launchResults[algo->getName()].push_back(stats);
        }
        item->release();
    }
    return launchResults;
}


} // namespace sea
