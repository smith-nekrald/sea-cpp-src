#pragma once

#include "../../types.h"
#include "../../input/input_data.h"
#include "../../input/input_links.h"
#include "../dual_variables.h"

#include <vector>
#include <unordered_map>

namespace sea {
namespace backend {

using std::vector;
using std::unordered_map;

struct LagrangianRelaxationIndex {
    vector<ui32> arcToLambdaIndex;
    ui32 lambdaCount = 0;
    ui32 muCount = 0;
};

void initLagrangianRelaxationIndex(const InputLinks& links,
        const InputData& input,
        LagrangianRelaxationIndex* index);
void initDuals(const LagrangianRelaxationIndex& index, DualVariables* dualVariables);

} // namespace backend

void writeToFile(const std::string& pathToFile, const backend::LagrangianRelaxationIndex& index);
void loadFromFile(const std::string& pathToFile, backend::LagrangianRelaxationIndex* index);

} // namespace sea
