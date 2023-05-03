#pragma once

#include "../../manager.h"
#include "index_map.h"
#include "../../algorithm/state.h"
#include "../dual_variables.h"

#include <cppad/ipopt/solve.hpp>

namespace sea {
namespace backend {

enum class MemoryStrategy {
    EXACT_COMPUTATION,
    LIMITED_MEMORY_BFGS,
    LIMITED_MEMORY_SR1,
};

enum class DerivativeStrategy {
    FORWARD,
    BACKWARD
};

enum class LinearSolver {
    MA27,
    MA57,
    MA86,
    MA97,
    MA77,
    WSMP,
    MUMPS,
};

struct IpoptBackendConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;

    double scale = 1.0;
    MemoryStrategy memoryManagement = MemoryStrategy::EXACT_COMPUTATION;
    DerivativeStrategy derivativeComputation = DerivativeStrategy::BACKWARD;
    LinearSolver solver = LinearSolver::MA27;
    int filePrintLevel = 5;
    int printLevel = 5;

    bool needMemory = false;
    bool needDescriptionsInIndex = false;

    std::string path_prefix = "";

// This group of parameters corresponds to warm/cold start.
    bool saveVariables = true;
// Switch on using enhanced or default version.
    bool useEnhancedVersion = true;

    double defaultUtilizationRatio = 1.0;
};


class IpoptBackend {
public:
    IpoptBackend(const IpoptBackendConfig& config);
    void moveDecisionToTime(
            TimeParameters timeParameters,
            DecisionManagerPtr decision,
            ConstActionManagerPtr action,
            double* objectiveEstimation = nullptr,
            DualVariables* duals = nullptr);
    DecisionManagerPtr bendersQuery(
            const vector<bool>& allotments,
            double* objectiveEstimation = nullptr,
            DualVariables* duals = nullptr);
    vector<bool> provideAllotments(
            double* objectiveEstimation = nullptr);
    void setIgnoreSpot(bool value) {
        ignoreSpot = value;
    }
    void setUtilizationRatio(double value) {
        utilizationRatio = value;
    }

protected:
    void initConstraintsLR(vector<double>* glowerPtr, vector<double>* gupperPtr);
    void initBoundsLR(vector<double>* vlowerPtr, vector<double>* vupperPtr);
    void recalculate(TimeParameters timeParameters,
            DecisionManagerPtr decisionManagerToWrite,
            ConstActionManagerPtr currentActionManager,
            bool writeDecision = true,
            vector<bool>* allotmentsToSelect = nullptr,
            double* objectiveEstimation = nullptr,
            DualVariables* duals = nullptr);

protected:
    IpoptBackendConfig config;
    IpoptIndexManagerPtr indexManager;
    bool canInitVariables;
    bool ignoreSpot;
    vector<double> lastVariables;
    double utilizationRatio;
};

template<typename T>
void updateLower(T& value,  T candidate) {
    value = std::max(value, candidate);
}

template<typename T>
void updateUpper(T& value, T candidate) {
    value = std::min(value, candidate);
}

void logOptions(const std::string& options);
std::string makeOptionsFromConfig(const IpoptBackendConfig& config);
void logConstraints(
        const vector<double>& vlower,
        const vector<double>& vupper,
        const vector<double>& glower,
        const vector<double>& gupper,
        const vector<double>& variables,
        const IpoptIndexMap& index,
        BackendType backendType = BackendType::IPOPT);

} // namespace backend
} // namespace sea
