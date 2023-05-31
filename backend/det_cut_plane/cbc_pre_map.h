/**
 * @file cbc_pre_map.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements structures for Deterministic Cutting Plane with Cbc solver.
 * @copyright (c) Smith School of Business, 2023
 */
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

/**
 * @brief Structure for storing variable index and coefficient together.
*/
struct CoefIndex {
    /// @brief Coefficient.
    double coef;
    /// @brief Index of the variable.
    unsigned index;
};

/**
 * @brief Structure for efficient work with Cbc solver.
 * Responsible for efficiently building Cbc problem.
 */
struct CbcPreMap {
public:
    /// @brief Vector with variable indices.
    std::vector<int> indices;
    /// @brief Vector representing matrix.
    std::vector<double> matrix;
    /// @brief Vector with coin-requested starts.
    std::vector<CoinBigIndex> starts;

    /// @brief Lower constraint bounds.
    std::vector<double> glower;
    /// @brief Upper constraint bounds.
    std::vector<double> gupper;

    /// @brief Lower variable bounds.
    std::vector<double> vlower;
    /// @brief Upper variable bounds.
    std::vector<double> vupper;

    /// @brief Coefficients in objective.
    std::vector<double> objective;
    /// @brief Coefficients and indices together.
    std::vector<std::vector<CoefIndex>> coefByVar;

    /// @brief Number of variables.
    int variableCount;
    /// @brief Number of constraints.
    int constraintCount;

public:    
    /**
     * @brief Constructor for manual initialization afterwards.
     */
    CbcPreMap() {}

    /**
     * @brief Constructor initializing part of the objects.
     * 
     * @param aVariableCount    Number of variables.
     * @param aConstraintCount  Number of constrints.
     */
    CbcPreMap(int aVariableCount, int aConstraintCount)
            : indices(aVariableCount * aConstraintCount)
            , matrix(aVariableCount * aConstraintCount)
            , starts(aVariableCount)
            , objective(aVariableCount)
            , coefByVar(aVariableCount) {}

    /**
     * @brief Initialization method. 
     * 
     * @param aVariableCount        Number of variables.
     * @param aConstraintCount      Number of constraints. 
     */
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

    /**
     * @brief Reserves space for new constraints.
     * 
     * @param newCount The number of new constraints.
     */
    void reserveNewConstraints(int newCount) {
        constraintCount += newCount;

        glower.resize(glower.size() + newCount, -std::numeric_limits<double>::max());
        gupper.resize(gupper.size() + newCount, std::numeric_limits<double>::max());
    }

    /**
     * @brief Logs Cbc constraints.
     * 
     * @param indexMap The Map to add when calling corresponding function.
     */
    void logCbcConstraints(const DcpIndexMap& indexMap) {
        logConstraints(vlower, vupper, glower, gupper, std::vector<double>(variableCount),
            indexMap, sea::BackendType::DET_CUT_PLANE);
    }

    /**
     * @brief Prepares Cbc solver.
     * 
     * @param solver The solver to prepare.
     */
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
        assert(static_cast<int>(vlower.size()) == variableCount);
        assert(static_cast<int>(vupper.size()) == variableCount);
        assert(static_cast<int>(objective.size()) == variableCount);
        assert(static_cast<int>(glower.size()) == constraintCount);
        assert(static_cast<int>(gupper.size()) == constraintCount);

        solver.loadProblem(variableCount, constraintCount, starts.data(), indices.data(),
                matrix.data(), vlower.data(), vupper.data(),
                objective.data(), glower.data(), gupper.data());

        solver.setObjSense(-1); // -> max
    }
};

} // namespace backend
} // namespace sea
