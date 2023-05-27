#pragma once

#include "../ipopt/ipopt_backend.h"
#include "../../manager.h"
#include "index_map.h"
#include "cbc_pre_map.h"

#include <cppad/ipopt/solve.hpp>

namespace sea {
namespace backend {

struct DetCutPlaneBackendConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;

    // These values must be equal!
    unsigned cbcLogLevel = 3;
    unsigned cbcFileLogLevel = 3;

    unsigned seed = 124124;
    unsigned iterations = 100;
    unsigned initialPlanes = 30;

    double needError = 0.01;

    // Shall algo stop when infeasible?
    bool stopOnInfeasible = false;

    // Tolerance.
    double integerTolerance = 1e-2;

    // Shall algo tolerate constraints?
    bool tolerateConstraints = true;

    bool needMemory = false;

    double defaultUtilizationRatio = 1.0;
};

class DetCutPlaneBackend {
public:
    DetCutPlaneBackend(const DetCutPlaneBackendConfig& config);

    DecisionManagerPtr provideAllotments(double* objectiveValue = nullptr);

    void setUtilizationRatio(double value) {
        utilizationRatio = value;
    }

private:
    void initConstraintsLR(
            vector<double>* glowerPtr, vector<double>* gupperPtr);
    void initBoundsLR(
            vector<double>* vlowerPtr, vector<double>* vupperPtr);

    void setupMainProblem();
    double runCbc();
    void addConstraints();
    void genRandomSolution();
    double calcError();
    void fillDecision(Decision* decision);

    double checkConstraintsAndBounds();

private:
    void logIterationInProvideAllotments(
            unsigned iterCount, double error, double constraintError,
            double accumulatedConstraintError, double objectiveEstimation) const;

private:
    CbcPreMap cbcLastProblem;

    DetCutPlaneBackendConfig config;
    DcpIndexManagerPtr indexManager;

    vector<double> lastSolution;
    double preparedSolutionObjective;
    double utilizationRatio;
};


} // namespace backend
} // namespace sea
