/**
 * @file gm_reg.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements regularizer. 
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "gm_interfaces.h"
#include "gm_common.h"

#include <vector>
#include <memory>

namespace gm {

/**
 * @brief Implements regularizer in the form of L2 squared metric, centered to a reference
 * center point.
 * 
 * @tparam Type The type of the vector to reqularize. Two cases expected: double and AD<double>.
 */
template<typename Type>
class L2SquaredReg : public IOrderOneOracle<Type> {
public:
    /**
     * @brief Constructor for the regularizer.
     * 
     * @param coeff Regularization coefficient. Smaller values are closer to the function,
     * but slow-down the optimization.
     * @param aCenter Center value for regularization centering.
     */
    L2SquaredReg(double coeff, std::vector<double> aCenter=std::vector<double>())
        : coeff(coeff)
        , center(aCenter) {
    };
    /**
     * @brief Gets function value in point. For this regularization, returns l2norm(x - center)^2.
     * 
     * @param point The point to compute function value at.
     * @return The function value in point.
     */
    virtual Type getValue(const std::vector<Type>& point) const {
        auto pointCopy = point;
        if (pointCopy.size() == center.size()) {
            for (size_t idx = 0; idx < adPoint.size(); ++idx) {
                pointCopy[idx] -= center[idx];
            }
        }
        return coeff * l2normSquared<Type>(pointCopy);
    };
    /**
     * @brief Computes subgradient in point. For this regularization, computes subgradient
     * from the expression l2norm(x - center)^2.
     * 
     * @param point The point to compute subgradient at.
     * @return Subgradient in point.
     */
    virtual std::vector<Type> getSubgradient(const std::vector<Type>& point) const {
        auto adPoint = makeADRoutine(point);
        auto adCopy = adPoint;
        if (adPoint.size() == center.size()) {
            for (size_t idx = 0; idx < adPoint.size(); ++idx) {
                adCopy[idx] -= center[idx];
            }
        }
        CppAD::AD<Type> entry = coeff * l2normSquared<CppAD::AD<Type>>(adCopy);
        std::vector<CppAD::AD<Type>> objective = {entry};
        return subgradientRoutine<Type>(point, adPoint, objective);
    };
    /**
     * @brief Virtual destructor for correct C++ polymorphism.
     */
    virtual ~L2SquaredReg() {};
private:
    /// @brief Regularization coefficient.
    double coeff;
    /// @brief Centering value. Reference point s.t. f(x) + reg(x) \approx f(x) at optimal point.
    std::vector<double> center;
};

} // namespace gm
