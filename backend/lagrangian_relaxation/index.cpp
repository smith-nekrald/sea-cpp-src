// Implments logic related to Lagrangian Relaxation Index.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "index.h"
#include "index.hpp"

#include <limits>


namespace sea {
namespace backend {

using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;

void initLagrangianRelaxationIndex(const InputLinks& links,
        const InputData& input, LagrangianRelaxationIndex* index) {
    unsigned arcCount = input.arcs.size();
    index->lambdaCount = 0;
    index->muCount = links.allotmentsWithItinerary.size();
    const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();
    index->arcToLambdaIndex.resize(arcCount, MAX_INDEX);
    index->arcToLambdaIndex.shrink_to_fit();
    for (const auto& arc: input.arcs) {
        if (arc.type == ArcType::travel) {
            index->arcToLambdaIndex[arc.id] = index->lambdaCount++;
        }
    }
}

void initDuals(const LagrangianRelaxationIndex& index, DualVariables* dualVariables) {
    const double INIT_LAMBDA = 0.01;
    dualVariables->lambdaVariables.assign(index.lambdaCount, INIT_LAMBDA);
    dualVariables->lambdaVariables.shrink_to_fit();

    const double INIT_MU = 0.01;
    dualVariables->muVariables.assign(index.muCount, INIT_MU);
    dualVariables->muVariables.shrink_to_fit();
}

} // namespace backend

void writeToFile(const std::string& pathToFile, const backend::LagrangianRelaxationIndex& index) {
    std::ofstream writer(pathToFile, std::ios::binary);
    ::cereal::BinaryOutputArchive outputArchive(writer);
    outputArchive << index;
}

void loadFromFile(const std::string& pathToFile, backend::LagrangianRelaxationIndex* index) {
    std::ifstream reader(pathToFile, std::ios::binary);
    ::cereal::BinaryInputArchive inputArchive(reader);
    inputArchive >> *index;
}

} // namespace sea
