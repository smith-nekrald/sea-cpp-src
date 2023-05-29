/**
 * @file gm_projector.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines and Implements projector - entity to project any point 
 * into the constraint-defined space.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>
#include <numeric>
#include <cppad/ipopt/solve.hpp>


namespace gm {

/**
 * @brief Entity to build objective and constraints for MinDistProjector.
 */
class ProjectorOptimizationProblem {
public:
    /// @brief Simplified notation for a CppAD point.
    typedef std::vector<CppAD::AD<double>> ADvector;

public:
    /**
     * @brief Constructor for MinDistProjector.
     * 
     * @param aQDescriptor Object to generate projection space constraints.
     * @param aPoint 
     */
    ProjectorOptimizationProblem(std::shared_ptr<const IQDescriptor> aQDescriptor,
            std::vector<double> aPoint)
        : qDescriptor(aQDescriptor)
        , point(aPoint) {
    }
    /**
     * @brief Builds objective and constraints from variables.
     * 
     * @param functions Array with objective (functions[0]) and constraints (functions[1:]).
     * @param variables Variables to use for building objective and constraints.
     */
    void operator()(ADvector& functions, const ADvector& variables) {
        auto constraints = qDescriptor->makeConstraints(variables);
        functions.resize(constraints.size() + 1);
        for (size_t idx = 0; idx < constraints.size(); ++idx) {
            functions[idx + 1] = constraints[idx];
        }
        CppAD::AD<double> distance = 0;
        for (size_t idx = 0; idx < variables.size(); ++idx) {
            distance += (point[idx] - variables[idx]) * (point[idx] - variables[idx]);
        }
        functions[0] = distance;
    }

private:
    /// @brief Object to generate projection space constraints.
    std::shared_ptr<const IQDescriptor> qDescriptor;
    /// @brief Point that needs to get projected.
    std::vector<double> point;
};


/**
 * @brief Class for projecting a point into constraint-defined subspace Q.
 * This projector looks for a point in Q with minimal l2-squared distance from original point.
 */
class MinDistProjector : public IQProjector {
public:
    /**
     * @brief Projector constructor.
     * 
     * @param aQDescriptor Description for projecting constraint-defined space.
     */
    MinDistProjector(std::shared_ptr<const IQDescriptor> aQDescriptor)
        : qDescriptor(aQDescriptor) {}
    /**
     * @brief Projects point onto constraint-defined space Q.
     * 
     * @param point  The point to project.
     * @return The projected point.
     */
    virtual std::vector<double> project(std::vector<double> point) const override {
        std::string options = "";
        options += "Sparse true reverse \n";
        options += "String linear_solver ma57 \n";
        options += "Integer max_iter     200 \n"; // default 3000
        options += "Numeric tol          1e-4 \n"; // default 1e-8
        options += "Integer print_level  0   \n"; // default is 5
        options += "String  sb           yes \n";

        std::vector<double> vlower, vupper, glower, gupper, variables = point;
        qDescriptor->initConstraintsLR(&glower, &gupper);
        qDescriptor->initBoundsLR(&vlower, &vupper);
        ProjectorOptimizationProblem problem(qDescriptor, point);
        CppAD::ipopt::solve_result<std::vector<double>> solution;
        CppAD::ipopt::solve<std::vector<double>, ProjectorOptimizationProblem>(
            options, variables,
            vlower, vupper,
            glower, gupper,
            problem, solution
        );
        variables = solution.x;
        return variables;
    };
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~MinDistProjector() {};

private:
    /// @brief Object to generate constraints that define projection space.
    std::shared_ptr<const IQDescriptor> qDescriptor;
};

} // namespace gm
