#include "index.h"

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
void serialize(Archive& archive, sea::backend::LagrangianRelaxationIndex& index) {
    archive(index.arcToLambdaIndex, index.lambdaCount, index.muCount);
}

} // namespace cereal


