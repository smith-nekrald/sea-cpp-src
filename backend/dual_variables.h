/**
 * @file dual_variables.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements Dual Variables structure: template and specification for double type.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <vector>
#include <unordered_map>

namespace sea {

using std::vector;
using std::unordered_map;

/**
 * @brief Template for dual variables and related structures. 
 * Contains two components: component that corresponds to lambda variables, and component that
 * corresponds to mu variables.
 * 
 * @tparam Type The type of inner values. Options: CppAD<double>, double, bool.
 */
template<class Type>
struct DualTemplate {
    /// @brief Components that correspond to lambda duals. The size = # of related arcs.
    vector<Type> lambdaVariables; 
    /// @brief Components that correspond to mu duals. The size = # of itineraries.
    vector<Type> muVariables;
};

/// @brief Particular case - template with double. Represents commonly used dual variables.
using DualVariables = DualTemplate<double>;

} // namespace sea
