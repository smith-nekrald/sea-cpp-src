/**
 * @file gm_common.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Commonly used functions and expressions.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <vector>
#include <cppad/ipopt/solve.hpp>
#include <memory>

namespace gm {

/**
 * @brief Computes subgradient from function expression. Requires domain (reference point for
 * computing expressions), CppAD point adPoint, and objective which is function expresion in point.
 * 
 * @tparam Type The type of value.
 * @param domain Reference point for computing expressions.
 * @param adPoint The variable CppAD point, independent variables.
 * @param objective The function evaluation at adPoint.
 * @return Sugradient in point. Expression or value.
 */
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

/**
 * @brief Sets background CppAD values to specific ones.
 * 
 * @tparam Type The value type to process.
 * @param domain Point for initialization.
 * @return Vector with initialized independent variables.
 */
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

/**
 * @brief Prepares initialization value.
 * 
 * @tparam Type The type of initialization value.
 * @return The value for initialization. Zero.
 */
template<typename Type> inline Type makeInit() {
    using BaseType=typename Type::AD::value_type;
    return BaseType(0);
}

/**
 * @brief Particular case of makeInit for double.
 * 
 * @tparam Template parameter is set to double.
 * @return The pre-initialization value. Zero.
 */
template<> inline double makeInit<double>() {
    return 0;
}

/**
 * @brief Template method for computing L2 squred norm.
 * 
 * @tparam Type The template type.
 * @param point The point to evaluate squared norm at.
 * @return The computed L2 squared norm at point.
 */
template<typename Type>
Type l2normSquared(const std::vector<Type>& point) {
    Type result = makeInit<Type>();
    for (const Type& item : point) {
        result += item * item;
    }
    return result;
}

} // namespace gm
