#pragma once

#include "../../common.h"
#include "../lagrangian_relaxation/index.h"
#include "../ipopt/ipopt_backend.h"
#include "index_map.h"

#include <coin-or/OsiSolverInterface.hpp>
#include <coin-or/OsiCbcSolverInterface.hpp>
#include <coin-or/OsiClpSolverInterface.hpp>

#include <iostream>
#include <limits>

namespace sea {
namespace backend {

struct CoefIndex {
    double coef;
    unsigned index;
};

struct CbcPreMap {
    std::vector<int> indices;
    std::vector<double> matrix;
    std::vector<CoinBigIndex> starts;

    std::vector<double> glower;
    std::vector<double> gupper;

    std::vector<double> vlower;
    std::vector<double> vupper;

    std::vector<double> objective;

    std::vector<std::vector<CoefIndex>> coefByVar;

    int variableCount;
    int constraintCount;

    CbcPreMap() {}

    CbcPreMap(int aVariableCount, int aConstraintCount)
            : indices(aVariableCount * aConstraintCount)
            , matrix(aVariableCount * aConstraintCount)
            , starts(aVariableCount)
            , objective(aVariableCount)
            , coefByVar(aVariableCount) {}

    void init(int aVariableCount, int aConstraintCount) {
        variableCount = aVariableCount;
        constraintCount = aConstraintCount;

        glower.clear();
        gupper.clear();
        vlower.clear();
        vupper.clear();

        starts.assign(aVariableCount + 1, 0);
        objective.assign(aVariableCount, 0);
        coefByVar.assign(aVariableCount, std::vector<CoefIndex>());

        starts.shrink_to_fit();
        objective.shrink_to_fit();
        coefByVar.shrink_to_fit();
    }

    void reserveNewConstraints(int newCount) {
        constraintCount += newCount;

        glower.resize(glower.size() + newCount, -std::numeric_limits<double>::max());
        gupper.resize(gupper.size() + newCount, std::numeric_limits<double>::max());
    }

    void logCbcConstraints(const DcpIndexMap& indexMap) {
        logConstraints(vlower, vupper, glower, gupper, std::vector<double>(variableCount),
            indexMap, sea::BackendType::DET_CUT_PLANE);
    }

    void setupCbcSolver(OsiCbcSolverInterface& solver) {
        unsigned matrixIndex = 0;
        indices.clear();
        matrix.clear();
        for (unsigned variableIndex = 0; variableIndex < coefByVar.size(); variableIndex++) {
            starts[variableIndex] = matrixIndex;
            for (auto& coeffIndex : coefByVar[variableIndex]) {
                matrix.push_back(coeffIndex.coef);
                indices.push_back(coeffIndex.index);
                matrixIndex++;
            }
        }

        starts[variableCount] = matrixIndex;

        matrix.shrink_to_fit();
        indices.shrink_to_fit();

        for (std::size_t idx = 0; idx < indices.size(); idx++) {
            assert(indices[idx] < constraintCount);
        }

        assert(matrixIndex == matrix.size());
        assert(int(vlower.size()) == variableCount);
        assert(int(vupper.size()) == variableCount);
        assert(int(objective.size()) == variableCount);
        assert(int(glower.size()) == constraintCount);
        assert(int(gupper.size()) == constraintCount);

        solver.loadProblem(variableCount, constraintCount, starts.data(), indices.data(),
                matrix.data(), vlower.data(), vupper.data(),
                objective.data(), glower.data(), gupper.data());

        solver.setObjSense(-1); // -> max
    }
};

} // namespace backend
} // namespace sea
