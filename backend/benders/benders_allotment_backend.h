#pragma once

#include "../../manager.h"
#include "../lagrangian_relaxation/lagrangian_relaxation_backend.h"
#include "../lagrangian_relaxation/structures.h"

#include <vector>
#include <utility>
#include <unordered_map>

namespace sea {
namespace backend {

using CoefficientUInfo = std::pair<UCoefficients, DualVariables>;

struct BendersAllotmentBackendConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
    ui32 cbcLogLevel;
    ui32 cbcFileLogLevel;

    bool needMemory = false;
    double globalPrecision = 1e-5;
};

struct DualStorage {
    std::vector<CoefficientUInfo> submittedDuals;
};

class BendersAllotmentBackend {
public:
    BendersAllotmentBackend(const BendersAllotmentBackendConfig& config);
    void addDuals(const DualVariables& duals);
    vector<bool> makeAllotments(DecisionManagerPtr decisionManager);

private:
    BendersAllotmentBackendConfig config;
    DualStorage storage;
    vector<double> lastSolution;
};

} // namespace backend
} // namespace sea
