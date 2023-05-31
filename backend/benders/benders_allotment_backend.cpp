// Implements BendersAllotmentBackend and related API.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "../lagrangian_relaxation/functions.h"
#include "benders_allotment_backend.h"

#include <coin-or/OsiSolverInterface.hpp>
#include <coin-or/OsiCbcSolverInterface.hpp>
#include <coin-or/OsiClpSolverInterface.hpp>

#include <vector>
#include <filesystem>


using std::size_t;

namespace sea {
namespace backend {

void recalculateUCoefficientsForDeterministic(
        const ConstInputManagerPtr& inputManager,
        const ConstLinksManagerPtr& linksManager,
        const ConstLRIndexManagerPtr& indexManager,
        const State& state,
        DecisionManagerPtr decisionManager,
        vector<CoefficientUInfo>* uinfo) {
    for (auto& entry : *uinfo) {
        computeSubgradientInPoint(
                entry.second, state, linksManager, inputManager,
                indexManager, decisionManager, &entry.first);
    }
}

void prepareBestUSolution(const vector<CoefficientUInfo>& duals,
    double* lastSolution, unsigned size, double precision) {
    double yValue = COIN_DBL_MAX;
    for (const auto & coeffUInfo : duals) {
        double rhs = 0;
        for (unsigned uInd = 0; uInd < coeffUInfo.first.coefficients.size(); ++uInd) {
            rhs += lastSolution[uInd] * coeffUInfo.first.coefficients[uInd];
        }
        rhs += coeffUInfo.first.freeMember;
        yValue = std::min<double>(yValue, rhs);
    }
    lastSolution[size - 1] = (yValue - precision);
}

BendersAllotmentBackend::BendersAllotmentBackend(const BendersAllotmentBackendConfig& aConfig)
    : config(aConfig) {
}

void BendersAllotmentBackend::addDuals(const DualVariables& duals) {
    CoefficientUInfo info;
    info.second = duals;
    // Setting dummy values to coefficients.
    info.first.coefficients.assign(config.inputManager->getConstData().allotments.size() , 0);
    info.first.freeMember = 0;
    storage.submittedDuals.push_back(info);
}

vector<bool> BendersAllotmentBackend::makeAllotments(DecisionManagerPtr basicDecision) {
    State state;
    initState(config.inputManager->getConstData(), &state);
    std::string filePath = "lr_index_make_allotments" + makeUniqueFileName();
    ManagerConfig managerConfig = {config.needMemory, filePath, true};

    auto indexManager = std::make_shared<DataManager<LagrangianRelaxationIndex>>(managerConfig);
    initLagrangianRelaxationIndex(
            config.linksManager->getConstData(),
            config.inputManager->getConstData(),
            indexManager->get());

    recalculateUCoefficientsForDeterministic(
        config.inputManager, config.linksManager, indexManager, 
        state, basicDecision, &storage.submittedDuals);
    indexManager.reset();

    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();
    auto& preparedDuals = storage.submittedDuals;

    // make problem
    int variablesCount = input.allotments.size() + 1;  // u, y
    // count of: y + C * u <= F(dual),
    // sum_by_group(u) <= 1
    int constraintsCount = preparedDuals.size() + input.allotmentGroups.size();
    int matrixElementsCount = variablesCount * preparedDuals.size();
    for (const auto & group : input.allotmentGroups) {
        matrixElementsCount += group.size();
    }

    vector<CoinBigIndex> starts(variablesCount + 1);

    vector<int> indices(matrixElementsCount);
    vector<double> matrix(matrixElementsCount);   // for constraints

    unsigned matrixPos = 0;
    // assign u coeffs
    for (unsigned uIdx = 0; uIdx < input.allotments.size(); ++uIdx) {
        starts[uIdx] = matrixPos;

        for (unsigned ind = 0; ind < preparedDuals.size(); ++ind) {
            indices[matrixPos] = ind;
            matrix[matrixPos++] = -preparedDuals[ind].first.coefficients[uIdx];
        }
        for (unsigned groupId : links.allotmentToGroups[uIdx]) {
            indices[matrixPos] = preparedDuals.size() + groupId;
            matrix[matrixPos++] = 1;
        }
    }

    // assign y coeffs
    starts[variablesCount - 1] = matrixPos;
    for (unsigned idx = 0; idx < preparedDuals.size(); ++idx) {
        indices[matrixPos] = idx;
        matrix[matrixPos++] = 1;
    }

    assert(static_cast<int>(matrixPos) == static_cast<int>(matrixElementsCount));
    starts[variablesCount] = matrixElementsCount;

    vector<double> constraintUpperBound(constraintsCount);
    vector<double> constraintLowerBound(constraintsCount);

    //assign constraints
    for (unsigned ind = 0; ind < preparedDuals.size(); ++ind) {
        constraintLowerBound[ind] = -COIN_DBL_MAX;
        constraintUpperBound[ind] = preparedDuals[ind].first.freeMember;
    }
    for (unsigned ind = 0; ind < input.allotmentGroups.size(); ++ind) {
        constraintLowerBound[preparedDuals.size() + ind] = 0;
        constraintUpperBound[preparedDuals.size() + ind] = 1;
    }

    vector<double> varUpperBound(variablesCount);
    vector<double> varLowerBound(variablesCount);

    // u
    for (unsigned uIdx = 0; uIdx < input.allotments.size(); ++uIdx) {
        varLowerBound[uIdx] = 0;
        varUpperBound[uIdx] = 1;
    }
    // y
    varLowerBound[variablesCount - 1] = -COIN_DBL_MAX;
    varUpperBound[variablesCount - 1] = COIN_DBL_MAX;

    vector<double> objective(variablesCount);
    // u
    for (unsigned uIdx = 0; uIdx < input.allotments.size(); ++uIdx) {
        objective[uIdx] = 0;
    }
    // y -> max
    objective[variablesCount - 1] = 1;

    // prepare solver
    OsiCbcSolverInterface solver;

    solver.loadProblem(variablesCount, constraintsCount, starts.data(), indices.data(),
            matrix.data(), varLowerBound.data(), varUpperBound.data(), objective.data(),
            constraintLowerBound.data(), constraintUpperBound.data());


    solver.setObjSense(-1); // -> max

    for (unsigned uIdx = 0; uIdx < input.allotments.size(); ++uIdx) {
        solver.setInteger(uIdx);
    }

    // get model
    // Previously was (seems bad -- gives exception):    auto model = solver.getModelPtr();
    CbcModel model(solver);
    unsigned cbcLogLevel = config.cbcLogLevel;
    model.setLogLevel(cbcLogLevel);

    // set previous solution
    if (lastSolution.size() != static_cast<size_t>(variablesCount)) {
        lastSolution.assign(variablesCount, 0);
    }

    prepareBestUSolution(
            preparedDuals, lastSolution.data(), variablesCount, config.globalPrecision);
    model.setBestSolution(
            lastSolution.data(), variablesCount, lastSolution.back() * solver.getObjSense());

    // solve
    {
            int fileDescriptor; fpos_t position;
            fflush(stdout);
            fgetpos(stdout, &position);
            fileDescriptor = dup(fileno(stdout));
            auto folderPath = std::filesystem::path("benders");
            if (!std::filesystem::exists(folderPath)) {
                std::filesystem::create_directories(folderPath);
            }
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wunused-result"
            freopen("benders/cbc.log", "a+", stdout);
            #pragma GCC diagnostic pop
            printf("Benders branch & bound. Started computation.\n");

            model.branchAndBound();

            printf("Benders branch & bound. Finished computation.\n");
            fflush(stdout);
            dup2(fileDescriptor, fileno(stdout));
            close(fileDescriptor);
            clearerr(stdout);
            fsetpos(stdout, &position);
    }

    auto solution = model.getColSolution();

    // save solution
    CoinDisjointCopyN(solution, variablesCount, lastSolution.data());
    vector<bool> response(input.allotments.size(), false);
    for (unsigned uIdx = 0; uIdx < input.allotments.size(); ++uIdx) {
        lastSolution[uIdx] = static_cast<unsigned>(round(solution[uIdx]));
        response[uIdx] = static_cast<unsigned>(round(solution[uIdx]));
    }
    return response;
}

} // namespace backend
} // namespace sea
