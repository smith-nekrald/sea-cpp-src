#include "lagrangian_relaxation_backend.h"
#include "functions.h"
#include "lr_cppad.h"
#include "gm_sea/sea_gm_apply.h"
#include "../../logging/logging.h"


namespace sea {
namespace backend {

using namespace gm;

LagrangianRelaxationBackend::LagrangianRelaxationBackend(
            const LagrangianRelaxationBackendConfig& aConfig)
        : config(aConfig) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.info("Entered LR constructor.");
    std::string filePath = "lr_index_" + makeUniqueFileName();
    ManagerConfig managerConfig = {config.needMemory.value(),
        filePath, true};
    index = std::make_shared<DataManager<
        LagrangianRelaxationIndex>>(managerConfig);
    logger.debug("Created LR:indexManager.");
    initLagrangianRelaxationIndex(
            config.linksManager->getConstData(),
            config.inputManager->getConstData(),
            index->get());
    logger.debug("LR index is initialized");
    randomPack.generator = std::default_random_engine(config.seed.value());
    randomPack.normalDist = std::normal_distribution<double>(
            config.normalMean.value(), config.normalStd.value());
    randomPack.uniformDist =
        std::uniform_real_distribution<double>(
                config.uniformMin.value(), config.uniformMax.value());
    randomPack.bernoullyDist =
        std::bernoulli_distribution(config.bernoullyProba.value());
    logger.info("Leaved LR constructor.");
}

DualVariables LagrangianRelaxationBackend::provideDuals(
            const State& state,
            DecisionManagerPtr decisionManager,
            UCoefficients* uCoeff,
            double* objectiveEstimation) const {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.debug("LR Backend -- Entered provideDuals");
    DualVariables duals;
    initDuals(index->getConstData(), &duals);
    if (config.optimizationAlgo == OptimizationAlgo::CUTTING_PLANE) {
        doCuttingPlaneOptimization(
                config.inputManager, config.linksManager, index, config, state,
                decisionManager, &randomPack, &duals, &dualHistory,
                objectiveEstimation, uCoeff, ignoreSpot, mean);
    } else if (config.optimizationAlgo == OptimizationAlgo::GM) {
        doGMOptimization(
                config.inputManager, config.linksManager, index, config, state,
                decisionManager, &randomPack, &duals, &dualHistory,
                objectiveEstimation, uCoeff, ignoreSpot, mean);
    } else {
        throw std::logic_error("Not implemented yet!");
    }
    logger.debug("Final duals are equal: ");
    printDualsToBackendLog(duals, BackendType::LR);

    logger.debug("LR Backend -- final decision");
    logger.debugStream() << decisionManager->getConstData();
    logger.debug("LR Backend -- Left provideDuals");
    mean = duals;
    return duals;
}

void LagrangianRelaxationBackend::makeCutoffDecisionWithDuals(
            const InputData::Event& event,
            const DualVariables& duals,
            const ConstActionManagerPtr& actionManager,
            DecisionManagerPtr decisionManager,
            State* state) const {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.debug("LR Backend -- asked to make CutoffDecisionWithDuals");
    logger.debug("Duals are equal to: ");
    printDualsToBackendLog(duals, BackendType::LR);
    assert(config.cutoffDecisionType == CutoffDecisionType::BY_ITINERARY);
    assert(state != nullptr);
    byItineraryRestore(event, config.inputManager, config.linksManager,
        index, actionManager, duals, state, decisionManager);
    logger.debug("LR Backend -- Leaving makeCutoffDecisionWithDuals");
}

} // namespace backend
} // namespace sea
