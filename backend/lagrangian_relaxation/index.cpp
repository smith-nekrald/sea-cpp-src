#include "index.h"

#include <limits>

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/base_class.hpp>

namespace cereal {

template<class Archive>
void serialize(Archive& ar, sea::backend::LagrangianRelaxationIndex& index)
{
    ar(index.arcToLambdaIndex,
        index.lambdaCount,
        index.muCount);
}

} // namespace cereal


const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();
const double INF = std::numeric_limits<double>::max();


namespace sea {
namespace backend {

using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;

void initLagrangianRelaxationIndex(const InputLinks& links,
        const InputData& input,
        LagrangianRelaxationIndex* index) {
    unsigned arcCount = input.arcs.size();
    index->lambdaCount = 0;
    index->muCount = links.allotmentsWithItinerary.size();
    index->arcToLambdaIndex.resize(arcCount, MAX_INDEX);
    index->arcToLambdaIndex.shrink_to_fit();
    for (const auto& arc: input.arcs) {
        if (arc.type == ArcType::travel) {
            index->arcToLambdaIndex[arc.id] = index->lambdaCount++;
        }
    }
}

void initDuals(const LagrangianRelaxationIndex& index, DualVariables* dualVariables) {
    dualVariables->lambdaVariables.assign(index.lambdaCount, 0.01);
    dualVariables->lambdaVariables.shrink_to_fit();

    dualVariables->muVariables.assign(index.muCount, 0.01);
    dualVariables->muVariables.shrink_to_fit();
}

} // namespace backend

void writeToFile(const std::string& pathToFile, const backend::LagrangianRelaxationIndex& index) {
    std::ofstream writer(pathToFile, std::ios::binary);
    ::cereal::BinaryOutputArchive oa(writer);
    oa << index;
}

void loadFromFile(const std::string& pathToFile, backend::LagrangianRelaxationIndex* index) {
    std::ifstream reader(pathToFile, std::ios::binary);
    ::cereal::BinaryInputArchive ia(reader);
    ia >> *index;
}

} // namespace sea
