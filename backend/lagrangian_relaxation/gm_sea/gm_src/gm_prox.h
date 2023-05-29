/**
 * @file gm_prox.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Prox-functions. Definitions and Implementations.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "gm_common.h"
#include "gm_interfaces.h"


#include <cppad/ipopt/solve.hpp>
#include <vector>
#include <memory>

namespace gm {

/**
 * @brief Prox function in the form d(x) = 0.5 * ||x||_2^2.
 * 
 * @tparam Type The type of point. Expected types are CppAD<double> and <double>.
 */
template<typename Type>
class ProxL2Squared : public IOrderOneOracle<Type> {
public:
    /**
     * @brief Prox-function constructor.
     */
    ProxL2Squared() {}
    /**
     * @brief Computes prox-function value at point.
     * 
     * @param point Point to compute prox-function value at.
     * @return The value of prox-function in point.
     */
    virtual Type getValue(const std::vector<Type>& point) const {
        return l2normSquared<Type>(point) / 2;
    }
    /**
     * @brief Computes prox-function subgradient at point.
     * 
     * @param point Point to compute subgradient at.
     * @return Subgradient at point.
     */
    virtual std::vector<Type> getSubgradient(const std::vector<Type>& point) const {
        std::vector<Type> result = point;
        return result;
    }
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~ProxL2Squared() {};
};


} // namespace gm

