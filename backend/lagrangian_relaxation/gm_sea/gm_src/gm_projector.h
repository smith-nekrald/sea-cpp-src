#pragma once

#include "gm_interfaces.h"

#include <vector>
#include <memory>
#include <numeric>
#include <cppad/ipopt/solve.hpp>


namespace gm {

class ProjectorOptimizationProblem {
public:
    typedef std::vector<CppAD::AD<double>> ADvector;

public:
    ProjectorOptimizationProblem(std::shared_ptr<const IQDescriptor> aQDescriptor,
            std::vector<double> aPoint)
        : qDescriptor(aQDescriptor)
        , point(aPoint) {
    }
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
    std::shared_ptr<const IQDescriptor> qDescriptor;
    std::vector<double> point;
};


class MinDistProjector : public IQProjector {
public:
    MinDistProjector(std::shared_ptr<const IQDescriptor> aQDescriptor)
        : qDescriptor(aQDescriptor) {
    }
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
    virtual ~MinDistProjector() {};
private:
    std::shared_ptr<const IQDescriptor> qDescriptor;
};


} // namespace gm
