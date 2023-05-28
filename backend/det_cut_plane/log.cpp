// Logging for the Det-Cut-Plane backend module.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "dcp_backend.h"
#include "../../logging/logging.h"

namespace sea {
namespace backend {

void DetCutPlaneBackend::logIterationInProvideAllotments(
        unsigned iterCount, double error, double constraintError,
        double accumulatedConstraintError, double objectiveEstimation) const {
    auto& logger = logging::getBackendSubLogger(BackendType::DET_CUT_PLANE);
    logger.getStream(log4cpp::Priority::INFO)
            << "Iteration: " << iterCount << ", " << "Error: " << error << ", "
            << "Constraint error: " << constraintError << ", " << "Accumulated error: "
            << accumulatedConstraintError << ", " << "Objective: " << objectiveEstimation;
}

}  // namespace backend
}  // namespace sea
