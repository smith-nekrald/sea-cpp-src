// String-related API.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2025

#include "ipopt_backend.h"

#include <filesystem>

namespace sea {
namespace backend {

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
    } else {
        throw std::logic_error("Unknown linear solver");
    }

    auto now = std::chrono::system_clock::now();
    auto inTimeT = std::chrono::system_clock::to_time_t(now);

    auto folderPath = std::filesystem::path(config.path_prefix);
    if  (!std::filesystem::exists(folderPath)) {
        std::filesystem::create_directories(folderPath);
    }
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

    options += "Integer max_iter     1000 \n"; // default 3000
    options += "Numeric tol          1e-3 \n"; // default 1e-8
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
}  // namespace sea
