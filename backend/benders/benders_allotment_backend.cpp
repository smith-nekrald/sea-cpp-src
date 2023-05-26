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

BendersAllotmentBackend::BendersAllotmentBackend(const BendersAllotmentBackendConfig& aConfig)
    : config(aConfig) {
}

void recalculateUCoefficientsForDeterministic(
        const ConstInputManagerPtr& inputManager,
        const ConstLinksManagerPtr& linksManager,
        const ConstLRIndexManagerPtr& indexManager,
        const State& state,
        DecisionManagerPtr decisionManager,
        vector<CoefficientUInfo>* uinfo) {
    for (auto& entry : *uinfo) {
        computeSubgradientInPoint(entry.second, state,
                linksManager, inputManager, indexManager, decisionManager, &entry.first);
    }
}



void BendersAllotmentBackend::addDuals(const DualVariables& duals) {
    CoefficientUInfo info;
    info.second = duals;
    // Setting dummy values to coefficients.
    info.first.coefficients.assign(config.inputManager->getConstData().allotments.size() , 0);
    info.first.freeMember = 0;
    storage.submittedDuals.push_back(info);
}

inline void prepareBestUSolution(const vector<CoefficientUInfo>& duals,
    double* lastSolution, unsigned size, double precision = 1e-5) {
    double y = COIN_DBL_MAX;
    for (const auto & coeffUInfo : duals) {
        double rhs = 0;
        for (unsigned u = 0; u < coeffUInfo.first.coefficients.size(); ++u) {
            rhs += lastSolution[u] * coeffUInfo.first.coefficients[u];
        }
        rhs += coeffUInfo.first.freeMember;
        y = std::min<double>(y, rhs);
    }
    lastSolution[size - 1] = (y - precision);
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
        config.inputManager, config.linksManager,
        indexManager,
        state,
        basicDecision,
        &storage.submittedDuals);
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
    for (unsigned u = 0; u < input.allotments.size(); ++u) {
        starts[u] = matrixPos;

        for (unsigned i = 0; i < preparedDuals.size(); ++i) {
            indices[matrixPos] = i;
            matrix[matrixPos++] = -preparedDuals[i].first.coefficients[u];
        }
        for (unsigned groupId : links.allotmentToGroups[u]) {
            indices[matrixPos] = preparedDuals.size() + groupId;
            matrix[matrixPos++] = 1;
        }
    }

    // assign y coeffs
    starts[variablesCount - 1] = matrixPos;
    for (unsigned i = 0; i < preparedDuals.size(); ++i) {
        indices[matrixPos] = i;
        matrix[matrixPos++] = 1;
    }

    assert(int(matrixPos) == int(matrixElementsCount));
    starts[variablesCount] = matrixElementsCount;

    vector<double> constraintUpperBound(constraintsCount);
    vector<double> constraintLowerBound(constraintsCount);

    //assign constraints
    for (unsigned i = 0; i < preparedDuals.size(); ++i) {
        constraintLowerBound[i] = -COIN_DBL_MAX;
        constraintUpperBound[i] = preparedDuals[i].first.freeMember;
    }
    for (unsigned i = 0; i < input.allotmentGroups.size(); ++i) {
        constraintLowerBound[preparedDuals.size() + i] = 0;
        constraintUpperBound[preparedDuals.size() + i] = 1;
    }

    vector<double> varUpperBound(variablesCount);
    vector<double> varLowerBound(variablesCount);

    // u
    for (unsigned u = 0; u < input.allotments.size(); ++u) {
        varLowerBound[u] = 0;
        varUpperBound[u] = 1;
    }
    // y
    varLowerBound[variablesCount - 1] = -COIN_DBL_MAX;
    varUpperBound[variablesCount - 1] = COIN_DBL_MAX;

    vector<double> objective(variablesCount);
    // u
    for (unsigned u = 0; u < input.allotments.size(); ++u) {
        objective[u] = 0;
    }
    // y -> max
    objective[variablesCount - 1] = 1;

    // prepare solver
    OsiCbcSolverInterface solver;

    solver.loadProblem(variablesCount, constraintsCount, starts.data(), indices.data(),
            matrix.data(), varLowerBound.data(), varUpperBound.data(), objective.data(),
            constraintLowerBound.data(), constraintUpperBound.data());


    solver.setObjSense(-1); // -> max

    for (unsigned u = 0; u < input.allotments.size(); ++u) {
        solver.setInteger(u);
    }

    // get model
    // Previously was (seems bad -- gives exception):    auto model = solver.getModelPtr();
    CbcModel model(solver);
    unsigned cbcLogLevel = config.cbcLogLevel;
    model.setLogLevel(cbcLogLevel);

    // set previous solution
    if (lastSolution.size() != size_t(variablesCount)) {
        lastSolution.assign(variablesCount, 0);
    }

    prepareBestUSolution(preparedDuals, lastSolution.data(),
                         variablesCount, config.globalPrecision);
    model.setBestSolution(lastSolution.data(), variablesCount,
                          lastSolution.back() * solver.getObjSense());
    // solve
    {
            int fd; fpos_t pos;
            fflush(stdout);
            fgetpos(stdout, &pos);
            fd = dup(fileno(stdout));
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
            dup2(fd, fileno(stdout));
            close(fd);
            clearerr(stdout);
            fsetpos(stdout, &pos);
    }

    auto solution = model.getColSolution();

    // save solution
    CoinDisjointCopyN(solution, variablesCount, lastSolution.data());
    vector<bool> response(input.allotments.size(), false);
    for (unsigned u = 0; u < input.allotments.size(); ++u) {
        lastSolution[u] =  unsigned(round(solution[u]));
        response[u] = unsigned(round(solution[u]));
    }
    return response;
}



} // namespace backend
} // namespace sea
