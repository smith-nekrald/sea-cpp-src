// Implements logging for the module.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "functions.h"
#include "lagrangian_relaxation_backend.h"
#include "../../logging/logging.h"

namespace sea {
namespace backend {

void printDualsToBackendLog(const DualVariables& duals, BackendType type) {
    auto& logger = logging::getBackendSubLogger(type);
    std::string muOutput = "MuVariables : ";
    for (const auto& mu: duals.muVariables) {
        muOutput += std::to_string(mu);
        muOutput += " ";
    }
    logger.debug(muOutput);
    std::string lambdaOutput = "LambdaVariables : ";
    for (const auto& lambda : duals.lambdaVariables) {
        lambdaOutput += std::to_string(lambda);
        lambdaOutput += " ";
    }
    logger.debug(lambdaOutput);
}

void logNonEmptyQInByItineraryRestore(unsigned itineraryId, double nonEmptyQ) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.debugStream() << "restore.cpp : itinerary = " << itineraryId;
    logger.debugStream() << "restore.cpp : Q = " << nonEmptyQ;
}

void logEnteredByItineraryRestore(
        const InputData::Event& event, const DualVariables& dualVariables) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.infoStream() << "Entered byItineraryRestore. \n " <<
            "Relative time = " << event.relativeTime;
        logger.debug("Duals to backend log: ");
        printDualsToBackendLog(dualVariables, BackendType::LR);

}

void logProcessingItineraryInByItineraryRestore(
        unsigned itineraryId, double lambdaSum, double qr, double zr) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.debug(
        "By itinerary restore. Processing itinerary with id" + std::to_string(itineraryId));
    logger.debugStream() << "byItineraryRestore: " <<  "Lambda Sum = " << lambdaSum;
    logger.debugStream() << "byItineraryRestore: " << "qr = " << qr << " zr = " << zr;
}

void logObjectiveAndBoundsInByItineraryRestore(const vector<double>& objective,
        const vector<double>& columnLower, const vector<double>& columnUpper) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    auto debugStream  = logger.getStream(log4cpp::Priority::DEBUG);
    debugStream << " Objective: \n";
    for (const auto& val : objective) {
        debugStream << val << " ";
    }
    debugStream << "\n Objective variables: \n";
    debugStream << "qr zr yrhs yros ...\n";
    debugStream << "ColumnLower: " << "\n";
    for (const auto& vl : columnLower) {
        debugStream << vl << " ";
    }
    debugStream << "\nColumnUpper: " << "\n";
    for (const auto& vu: columnUpper) {
        debugStream << vu << " ";
    }
}

void logMinCapacityArc(double minCapacityArc) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.debugStream() << "minCapacityArc = " << minCapacityArc << "\n";
}

void LagrangianRelaxationBackend::logLRConstruction() const {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.info("LR construction is Finished.");
}

void LagrangianRelaxationBackend::logMakeCutoffDecisionWithDuals(
        const DualVariables& duals) const {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.debug("LR Backend -- CutoffDecisionWithDuals");
    logger.debug("Duals are equal to: ");
    printDualsToBackendLog(duals, BackendType::LR);
}

void LagrangianRelaxationBackend::logProvideDuals(
        ConstDecisionManagerPtr decisionManager, const DualVariables& duals) const {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.debug("LR Backend -- In provideDuals");
    logger.debug("Final duals are equal: ");
    printDualsToBackendLog(duals, BackendType::LR);
    logger.debug("LR Backend -- final decision");
    logger.debugStream() << decisionManager->getConstData();
    logger.debug("LR Backend -- Left provideDuals");
}

void logEntranceToCheckIfFeasible(
        const DualVariables& variables,
        const std::vector<double>& boundLower,
        const std::vector<double>& boundUpper,
        const std::vector<double>& lhs) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.debug("Entered checkIfFeasible.");


    logger.debug("Received duals: ");
    printDualsToBackendLog(variables, BackendType::LR);

    logger.debug("BoundLower: ");
    printArrayToBackendLog(boundLower, BackendType::LR);
    logger.debug("BoundUpper: ");
    printArrayToBackendLog(boundUpper, BackendType::LR);
    logger.debug("lhs: ");
    printArrayToBackendLog(lhs, BackendType::LR);

}

void logLowerBoundFailureInCheckIfFeasible(double difference) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.notice("Check If feasible. Failure on lower bound inequality");
    logger.notice("Difference = " + std::to_string(difference));
}

void logUpperBoundFailureInCheckIfFeasible(double difference) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.notice("Check if feasible. Failure on upper bound inequality.");
    logger.notice("Difference = " + std::to_string(difference));
}

void logLhsInequalityFailureInCheckIfFeasible(double error) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.notice("Check If Feasible. Failure on lhs inequality.");
    logger.notice("Error = " + std::to_string(error));
}

void logExitFromCheckIfFeasible(const std::string& value) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.debug("Leaving check if Feasible. Decision is: ");
    logger.debug(value);
}

void logSubgradient(const SubgradientOptimizationParameters& current) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    auto localDebugStream = logger.debugStream();
    localDebugStream << "Final subgradient equals: \n";
    localDebugStream << "muSubgradient: \n";
    for (const auto& item : current.muSubgradient) {
        localDebugStream << item << " ";
    }
    localDebugStream << "lambdaSubgradient: \n";
    for (const auto& item : current.lambdaSubgradient) {
        localDebugStream << item << " ";
    }
    localDebugStream << "algo -- functionValue: ";
    localDebugStream << current.functionValue;
}

void logHitMuAndLambda(const vector<int>& hitMu, const vector<int>& hitLambda) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    auto stream = logger.getStream(log4cpp::Priority::NOTICE);
    stream << "Bound: hitLambda. \n";
    for (auto& idx : hitLambda) {
        stream << idx << " ";
    }
    stream << "\n Bound: hitMu. \n";
    for (auto& idx : hitMu) {
        stream << idx << " ";
    }
}

void logObjectiveAndDualNorms(double cuttingPlaneObjective, double regObjectiveValue,
        const DualVariables& dualVariables, const DualVariables& prevDuals) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.noticeStream() << "cutting_plane_objective = " << cuttingPlaneObjective;
    logger.noticeStream() << "reg_cppad_objective = " << regObjectiveValue;
    logger.noticeStream() << " L2 norm of duals = " << dualL2Norm(dualVariables);
    logger.noticeStream() << " L1 norm of duals = " << dualL1Norm(dualVariables);
    logger.noticeStream() << " L1 norm of subsequent " << "difference = "
            << dualAbsDiffSum(prevDuals, dualVariables) ;
}

void logDualHistorySize(std::size_t historySize) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.noticeStream() << "Size of dualHistory = " << historySize;
}

void logCuttingPlaneIterationDuals(
        unsigned iter, const DualVariables& dualVariables, bool feasible) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.noticeStream() << "Iteration: " << iter;
    logger.debug("New obtained dual variables (will be checked to be feasible): ");
    printDualsToBackendLog(dualVariables, BackendType::LR);
    logger.debug("Next call addDualsToSimplex with duals.");
    if (!feasible) {
        logger.error("Infeasible solution obtained with CBC cutting plane!");
    }
}

void logStartCuttingPlaneOptimization(const State& state, DualVariables* dualPtr) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.notice("Started CuttingPlaneOptimization.");
    auto debugStream = logger.getStream(log4cpp::Priority::DEBUG);
    printState(state, debugStream);
    debugStream << "\nPointer to dualVariables equals: " << dualPtr << "should be non-zero \n";
}

void logPlaneIntersectionItineraries(const vector<int>& hitIdx) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    auto stream = logger.getStream(log4cpp::Priority::NOTICE);
    stream << "Plane intersection itineraries:";
    for (auto& idx : hitIdx) {
        stream << " " << idx;
    }
}

void logFinishedCuttingPlaneIteration(
        unsigned iter, double absObjectiveDiff, double absShiftDiff, double relShiftDiff) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.noticeStream() << "Finished Iteration: " << iter << " obj_diff: "
        << absObjectiveDiff << " abs_shift_diff: " << absShiftDiff
        << " rel_shift_diff: " << relShiftDiff;
}

void logChangeInCuttingPlane(
        const std::pair<size_t, size_t>& lowerChange,
        const std::pair<size_t, size_t>& upperChange,
        const std::pair<size_t, size_t>& upperHitCount,
        const std::pair<size_t, size_t>& lowerHitCount,
        size_t targetMuCount,
        size_t targetLambdaCount,
        const vector<bool>& prevPlaneHits,
        const vector<bool>& currentPlaneHits) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.noticeStream() << "Lower bound change! " << "lambda_variables : "
        << lowerChange.first << " mu_variables : " << lowerChange.second <<
        " of " << targetLambdaCount + targetMuCount;
    logger.noticeStream() << "Upper bound change! " << "lambda_variables : "
        << upperChange.first << " mu_variables : " << upperChange.second
        << " of " << targetLambdaCount + targetMuCount;
    logger.noticeStream() << "Sum of hit changes: " << lowerChange.first + lowerChange.second +
        upperChange.first + upperChange.second << " of " << targetLambdaCount + targetMuCount;
    logger.noticeStream() << "Hit stats! " << "\nupper_lambda: " << upperHitCount.first <<
        " upper_mu: " << upperHitCount.second << "\nlower_lambda: " << lowerHitCount.first <<
        " lower_mu: " << lowerHitCount.second << "\nsum_of_hits" << upperHitCount.first +
            upperHitCount.second + lowerHitCount.first + lowerHitCount.second;
    logger.noticeStream() << "Plane Hits change : " <<
        computeHitDiff(prevPlaneHits, currentPlaneHits) << " of " << targetMuCount;;
    logger.noticeStream() << "Count of current plane hits: " <<
        computeTrueCount(currentPlaneHits) << " of " << targetMuCount;
}

void logLastCuttingPlaneIteration(unsigned iter) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.notice("The last Iteration was: " + std::to_string(iter));
}

void logRestartSimplexFromDeque() {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.noticeStream() << "Restarting simplex from deque.";
}

void logImmortalProcessed(bool writeLogOut, const DualDequeInfo& info) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    if (writeLogOut) {
        logger.notice("This immortal item is processed.");
        if (info.checked) {
            logger.notice("Item is now fixed.");
        } else {
            logger.notice("Item was not fixed.");
        }
    }
}

void logImmortalNorms(const DualDequeInfo& info) {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.noticeStream() << "Immortal L1 norm = " << dualL1Norm(info.duals);
    logger.noticeStream() << "Immortal L2 norm = " << dualL2Norm(info.duals);
}

void logProcessingImmortalUnchecked() {
    auto& logger = logging::getBackendSubLogger(BackendType::LR);
    logger.notice("Processing immortal and unchecked item.");
}

} // namespace backend
} // namespace sea
