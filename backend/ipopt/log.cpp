#include "ipopt_backend.h"
#include "optimization_problem.h"
#include "../lagrangian_relaxation/index.h"
#include "../../logging/logging.h"

#include <string>
#include <limits>
#include <memory>

namespace sea {
namespace backend {

using std::unordered_map;
using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;
using std::cout;
using std::endl;
using std::size_t;

const double INF = std::numeric_limits<double>::max();

void logConstraints(
        const vector<double>& vlower,
        const vector<double>& vupper,
        const vector<double>& glower,
        const vector<double>& gupper,
        const vector<double>& variables,
        const IpoptIndexMap& index,
        BackendType backendType) {
    assert(index.variableCount == vlower.size() &&
            index.variableCount == vupper.size() &&
            index.variableCount == variables.size());
    assert(index.constraintCount == glower.size() &&
        index.constraintCount == gupper.size());
    auto& logger = logging::getBackendSubLogger(backendType);
    logger.debug("Variable constraints: ");
    for (size_t idx = 0; idx < index.variableCount; ++idx) {
        logger.debug(
                std::string("Variable ") + std::to_string(idx)
                + ": ");
        if (index.variableToDescription.find(idx) !=
                index.variableToDescription.end()) {
            logger.debug(index.variableToDescription.at(idx));
        } else {
            logger.debug("No description.");
        }
        logger.debug(std::to_string(vlower[idx]) + " = lower");
        logger.debug(std::to_string(vupper[idx]) + " = upper");
        logger.debug(std::to_string(variables[idx]) + " = value");
        if (vlower[idx] > vupper[idx]) {
            logger.error(
                    "Lower is greater than upper for variable IDX "
                    + std::to_string(idx));
        }
    }
    logger.debug("Equation constraints: ");
    for (size_t idx = 0; idx < index.constraintCount; ++idx) {
        logger.debug(
                std::string("Constraint ") + std::to_string(idx)
                + ": ");
        if (index.constraintToDescription.find(idx) !=
                index.constraintToDescription.end()) {
            logger.debug(index.constraintToDescription.at(idx));
        } else {
            logger.debug("No description.");
        }
        logger.debug(std::to_string(glower[idx]) + " = lower");
        logger.debug(std::to_string(gupper[idx]) + " = upper");
        if (glower[idx] > gupper[idx]) {
            logger.error(
                    "Lower is greater than upper for constraint IDX "
                    + std::to_string(idx));
        }

    }
}

// Logging Functions
void logOptions(const std::string& options) {
   logging::getBackendSubLogger(sea::BackendType::IPOPT).debug(
            "Ipopt Options: " + options);
}

// Useful functions.
std::string makeOptionsFromConfig(const IpoptBackendConfig& config) {
    std::string options = "";
    if (config.derivativeComputation == DerivativeStrategy::FORWARD) {
        options += "Sparse true forward \n";
    } else {
        options += "Sparse true reverse \n";
    }

    if (config.solver == LinearSolver::MA27) {
        options += "String linear_solver ma27 \n";
    } else if (config.solver == LinearSolver::MUMPS) {
        options += "String linear_solver mumps \n";
    } else if (config.solver == LinearSolver::MA57) {
        options += "String linear_solver ma57 \n";
    } else if (config.solver == LinearSolver::MA86) {
        options += "String linear_solver ma86 \n";
    } else if (config.solver == LinearSolver::MA97) {
        options += "String linear_solver ma97 \n";
    } else if (config.solver == LinearSolver::MA77) {
        options += "String linear_solver ma77 \n";
    } else if (config.solver == LinearSolver::WSMP) {
        options += "String linear_solver wsmp \n";
    } else {
        throw std::logic_error("Unknown linear solver");
    }

    auto now = std::chrono::system_clock::now();
    auto inTimeT = std::chrono::system_clock::to_time_t(now);
    auto textVal = config.path_prefix + "/" + "ipopt_" + std::to_string(inTimeT) + ".log";

    options += "String output_file " + textVal + " \n";
    options += "Integer file_print_level " + std::to_string(config.filePrintLevel) + " \n";
    options += "String print_timing_statistics yes \n";
    options += "Integer print_level " +  std::to_string(config.printLevel) + " \n";

    if (config.memoryManagement == MemoryStrategy::LIMITED_MEMORY_BFGS) {
        options += "String hessian_approximation limited-memory \n";
        options += "String limited_memory_update_type bfgs \n";
    } else if (config.memoryManagement == MemoryStrategy::LIMITED_MEMORY_SR1) {
        options += "String hessian_approximation limited-memory \n";
        options += "String limited_memory_update_type sr1 \n";
    } else if (config.memoryManagement == MemoryStrategy::EXACT_COMPUTATION) {
        // Do nothing.
        // Since default options satisfy requirement.
    } else {
        throw std::logic_error("unknown MemoryStrategy");
    }

    options += "Integer max_iter     100 \n"; // default 3000
    options += "Numeric tol          1e-6 \n"; // default 1e-8
    options += "String  sb           yes \n";

    // In case problem is infeasible, the following options can help:
    // options += "Numeric bound_relax_factor 0 \n";
    // options += "String expect_infeasible_problem yes \n";

    // This option will print all possible options to stdout.
    // options += "String print_options_documentation yes \n";
    //
     return options;
}

} // namespace backend
} //namespace sea
