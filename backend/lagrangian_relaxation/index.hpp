/**
 * @file index.hpp
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Serialization for LR Index.
 * @copyright (c) Smith School of Business, 2023
 */
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

/**
 * @brief Template serialization for LR Index.
 * 
 * @tparam Archive Type of serialization archive.
 * @param archive Archive for serialization.
 * @param index LR Index to serialize.
 */
template<class Archive>
void serialize(Archive& archive, sea::backend::LagrangianRelaxationIndex& index) {
    archive(index.arcToLambdaIndex, index.lambdaCount, index.muCount);
}

} // namespace cereal


