#include "ipopt_backend.h"
#include "optimization_problem.h"
#include "../lagrangian_relaxation/index.h"
#include "../../logging/logging.h"

#include <string>
#include <limits>
#include <memory>

namespace sea {
namespace backend {

using std::unordered_map;
using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;
using std::cout;
using std::endl;
using std::size_t;

const double INF = std::numeric_limits<double>::max();

IpoptBackend::IpoptBackend(const IpoptBackendConfig& aConfig)
        : config (aConfig) {
    std::string filePath = "ipopt_index_map_" + makeUniqueFileName();
    ManagerConfig indexMapConfig = {config.needMemory, filePath, true};
    indexManager = std::make_shared<DataManager<IpoptIndexMap>>(indexMapConfig);
    createIndexMap(config.inputManager->getConstData(),
            indexManager->get(), config.useEnhancedVersion, config.needDescriptionsInIndex);
    auto ipoptLogger = logging::getBackendSubLogger(BackendType::IPOPT)
        << log4cpp::Priority::DEBUG;
    printIndexMapStats(ipoptLogger, indexManager->getConstData());
    canInitVariables = false;
    lastVariables.clear();
    ignoreSpot = false;
    utilizationRatio = config.defaultUtilizationRatio;
}

void IpoptBackend::moveDecisionToTime(
        TimeParameters timeParameters,
        DecisionManagerPtr decisionManager,
        ConstActionManagerPtr actionManager,
        double* objectiveEstimation,
        DualVariables* duals) {
    recalculate(timeParameters, decisionManager, actionManager,
            true, nullptr, objectiveEstimation, duals);
}

vector<bool> IpoptBackend::provideAllotments(double* objectiveEstimation) {
    auto& logger = logging::getBackendSubLogger(BackendType::IPOPT);
    logger.debugStream() << "Entered provideAllotments. ";

    std::string filePath = "decision_" + makeUniqueFileName();
    ManagerConfig decisionConfig = {config.needMemory, filePath, true};
    TimeParameters timeParameters;
    timeParameters.allotmentsSupplied = false;
    timeParameters.timeEvent = 0;
    vector<bool> answer;
    logger.debugStream() << "Redirecting to recalculate with empty answer.";
    recalculate(timeParameters, nullptr, nullptr, false, &answer, objectiveEstimation);
    {
        auto stream = logger.getStream(log4cpp::Priority::DEBUG);
        stream  << "The following answer is returned in provideAllotments \n";
        stream << "size = " << answer.size() << "\n";
        for (const auto& v : answer) {
            stream << v << " ";
        }
    }

    return answer;
}

DecisionManagerPtr IpoptBackend::bendersQuery(const vector<bool>& selectedAllotments,
         double* objectiveEstimation, DualVariables* duals) {
    std::string filePath = "decision_" + makeUniqueFileName();
    ManagerConfig decisionConfig = {config.needMemory, filePath, true};

    DecisionManagerPtr decisionManager = std::make_shared<DataManager<Decision>>(decisionConfig);
    createDecision(config.inputManager->getConstData(), decisionManager->get());
    assert(config.inputManager->getConstData().allotments.size() == selectedAllotments.size());
    decisionManager->get()->allotmentAccepted = selectedAllotments;

    TimeParameters timeParameters;
    timeParameters.allotmentsSupplied = true;
    timeParameters.timeEvent = 0;

    assert(config.inputManager->getConstData().allotments.size() == selectedAllotments.size());
    decisionManager->get()->allotmentAccepted = selectedAllotments;

    recalculate(timeParameters, decisionManager, nullptr, true, nullptr,
            objectiveEstimation, duals);
    return decisionManager;
}


} // namespace backend
} // namespace sea

