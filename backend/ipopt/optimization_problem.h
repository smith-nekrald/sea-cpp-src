#pragma once

#include "../../manager.h"
#include "index_map.h"
#include <cppad/ipopt/solve.hpp>

namespace sea {
namespace backend {

struct OptimizationConfig {
    ConstIpoptIndexManagerPtr indexManager;
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;

    ConstDecisionManagerPtr decisionManager;
    ConstActionManagerPtr actionManager;

    unsigned updateTime = 0;

    bool useEnhancedVersion = true;
    double utilizationRatio = 1.0;
};

class OptimizationProblem {
public:
    typedef vector<CppAD::AD<double>> ADvector;

public:
    OptimizationProblem(
            const OptimizationConfig& aConfig);
    void operator()(
            ADvector& functions,
            const ADvector& variables);

private:
    const OptimizationConfig config;
};

} // namespace backend
} // namespace sea
