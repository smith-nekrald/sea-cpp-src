#pragma once

#include <vector>
#include <unordered_map>

namespace sea {

using std::vector;
using std::unordered_map;

template<class Type>
struct DualTemplate {
    vector<Type> lambdaVariables; // size = # of related arcs
    vector<Type> muVariables;     // size = # of itineraries
};

using DualVariables = DualTemplate<double>;

} // namespace sea
