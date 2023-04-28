#pragma once

#include "../../common.h"
#include "../lagrangian_relaxation/index.h"
#include "index_map.h"

#include "OsiSolverInterface.hpp"
#include "OsiCbcSolverInterface.hpp"
#include "OsiClpSolverInterface.hpp"

#include <iostream>
#include <limits>

namespace sea {
namespace backend {

struct CoefIndex {
    double coef;
    ui32 index;
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

    CbcPreMap(int varCount, int constrCount)
            : indices(varCount * constrCount)
            , matrix(varCount * constrCount)
            , starts(varCount)
            , objective(varCount)
            , coefByVar(varCount)
    {}

    void init(int varCount, int constrCount) {
        variableCount = varCount;
        constraintCount = constrCount;

        glower.clear();
        gupper.clear();
        vlower.clear();
        vupper.clear();

        starts.assign(varCount + 1, 0);
        objective.assign(varCount, 0);
        coefByVar.assign(varCount, std::vector<CoefIndex>());

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
        ui32 matrixIndex = 0;
        indices.clear();
        matrix.clear();
        for (ui32 varIndex = 0; varIndex < coefByVar.size(); varIndex++) {
            starts[varIndex] = matrixIndex;
            for (auto& coefIndex : coefByVar[varIndex]) {
                matrix.push_back(coefIndex.coef);
                indices.push_back(coefIndex.index);
                matrixIndex++;
            }
        }

        starts[variableCount] = matrixIndex;

        matrix.shrink_to_fit();
        indices.shrink_to_fit();

        for (std::size_t i = 0; i < indices.size(); i++) {
            assert(indices[i] < constraintCount);
        }

        assert(matrixIndex == matrix.size());
        assert(int(vlower.size()) == variableCount);
        assert(int(vupper.size()) == variableCount);
        assert(int(objective.size()) == variableCount);
        assert(int(glower.size()) == constraintCount);
        assert(int(gupper.size()) == constraintCount);

        solver.loadProblem(variableCount, constraintCount, starts.data(), indices.data(), matrix.data(),
            vlower.data(), vupper.data(), objective.data(), glower.data(), gupper.data());

        solver.setObjSense(-1); // -> max
    }
};

} // namespace backend
} // namespace sea
