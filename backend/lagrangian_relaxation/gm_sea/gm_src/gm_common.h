#pragma once

#include <vector>
#include <cppad/ipopt/solve.hpp>
#include <memory>

namespace gm {

template<typename Type>
std::vector<Type> subgradientRoutine(
        const std::vector<Type>& domain,
        const std::vector<CppAD::AD<Type>>& adPoint,
        const std::vector<CppAD::AD<Type>>& objective) {
    CppAD::ADFun<Type> map(adPoint, objective);
    std::vector<Type> subgradient(domain.size());
    subgradient = map.Jacobian(domain);
    return subgradient;
}

template<typename Type>
std::vector<CppAD::AD<Type>> makeADRoutine(const std::vector<Type>& domain) {
    CppAD::AD<Type>::abort_recording();
    std::vector<CppAD::AD<Type>> adPoint(domain.size());
    for (std::size_t idx = 0; idx < domain.size(); ++idx) {
        adPoint[idx] = domain[idx];
    }
    CppAD::Independent(adPoint);
    return adPoint;
}

template<typename Type> inline Type makeInit() {
    using BaseType=typename Type::AD::value_type;
    return BaseType(0);
}

template<> inline double makeInit<double>() {
    return 0;
}

template<typename Type>
Type l2normSquared(const std::vector<Type>& point) {
    Type result = makeInit<Type>();
    for (const Type& item : point) {
        result += item * item;
    }
    return result;
}

} // namespace gm
