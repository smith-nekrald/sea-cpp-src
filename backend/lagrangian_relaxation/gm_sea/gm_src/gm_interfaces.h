/**
 * @file gm_interfaces.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines interfaces for commonly used optimization objects.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>

#include <cppad/ipopt/solve.hpp>

namespace gm {

/**
 * @brief Interface for a general-purpose gradient optimization method.
 */
class IGradientOptimizer {

public:
    /**
     * @brief IGradientOptimizer constructor.
     * 
     * @param flagKeepStory If true, optimizer keeps measurements history.
     * @param aName The name of the optimizer.
     */
    IGradientOptimizer(bool flagKeepStory, std::string aName)
        : keepStory(flagKeepStory)
        , name(aName) {}
    /**
     * @brief Method to start iterative optimization. Pure virtual function.
     * 
     * @param init Initial point to start optimization from.
     * @return Optimal point of algorithm converged. Some approximation otherwise.
     */
    virtual std::vector<double> optimize(const std::vector<double>& init) const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~IGradientOptimizer() {};
    /**
     * @brief Get the measurements history.
     * 
     * @return The measurements history. Map from measurement name to a vector with measurements.
     */
    std::map<std::string, std::vector<double>> getStory() const {
        return story;
    }
    /**
     * @brief Retrieves algorithm name.
     * 
     * @return The name of the algorithm.
     */
    std::string getName() const {
        return name;
    }
    /**
     * @brief Checks if story is recorded.
     * 
     * @return If history is recorded, returns true. Returns false otherwise.
     */
    bool isStoryKept() const {
        return keepStory;
    }

protected:
    /// @brief Whether to keep history of measurments.
    bool keepStory;
    /// @brief The name of the algorithm.
    const std::string name;
    /// @brief Place to write history of measurements.
    mutable std::map<std::string, std::vector<double>> story;
};

/**
 * @brief Zero-order oracle interface. Ensures opportunity to evaluate value in point.
 * 
 * @tparam Value The type of vector value. Two options expected: double and CppAD<double>.
 */
template<typename Value>
class IOrderZeroOracle {
public:
    /**
     * @brief Get value at point. Pure virtual function.
     * 
     * @param point The point to compute value at.
     * @return Calculated function value at point.
     */
    virtual Value getValue(const std::vector<Value>& point) const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~IOrderZeroOracle() {}
};

/**
 * @brief First-order oracle interface. Ensures opportunities to evaluate function
 * value and function subgradient in point.
 * 
 * @tparam Value The type of vector value. Two options supported: double and CppAD<double>.
 */
template<typename Value>
class IOrderOneOracle : public IOrderZeroOracle<Value> {
public:
    /**
     * @brief Gets the function subgradient at point. Pure virtual function.
     * 
     * @param point The point to compute subgradient at.
     * @return The function subgradient at point.
     */
    virtual std::vector<Value> getSubgradient(const std::vector<Value>& point) const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~IOrderOneOracle() {}
};

/// @brief Simple notation for double first-order oracle.
using DoubleFunction=IOrderOneOracle<double>;
/// @brief Simple notation for CppAD-typed first-order oracle.
using CppADFunction=IOrderOneOracle<CppAD::AD<double>>;
/// @brief Simple notation for a point in optimization space.
using ThePoint=std::vector<double>;

/**
 * @brief Computes Bregman Distance. Bregman distance is defined from prox-function as
 * \xi(x,y) = d(y) - d(x) - scalar_product(gradient(d(x)), y - x), where d(x) denotes 
 * the prox-function. Defined on slide 13 at second lecture from Yuri Nesterov HSE course.
 * 
 * @tparam ValueX The type of x in expression d(x). Different types are required since one
 * evaluation may require CppAD, while the other may require no CppAD.
 * @tparam ValueY The type of y in expression d(y). Different types are required since one
 * evaluation my require CppAD, while the other may require no CppAD.
 */
template<typename ValueX, typename ValueY>
class BregmanDistance {
public:
    /**
     * @brief Construct a new Bregman Distance object
     * 
     * @param xProx Object to evaluate prox function d(x) and to compute subgradient.
     * @param yProx Object to evaluate prox function d(y) and to compute subgradient.
     */
    BregmanDistance(std::shared_ptr<const IOrderOneOracle<ValueX>> xProx,
            std::shared_ptr<const IOrderOneOracle<ValueY>> yProx)
        : xOracle(xProx)
        , yOracle(yProx) {
    }
    /**
     * @brief Compute Bregman Distance xi(x, y).
     * 
     * @param xPoint Point x for computing xi(x, y).
     * @param yPoint Point y for computing xi(x, y).
     * @return The value of Bregman Distance.
     */
    ValueY getDistance(const std::vector<ValueX>& xPoint,
            const std::vector<ValueY>& yPoint) const {
        auto size = xPoint.size();
        assert(xPoint.size() == yPoint.size());
        auto answer = yOracle->getValue(yPoint) - xOracle->getValue(xPoint);

        std::vector<ValueX> grad;
        grad = xOracle->getSubgradient(xPoint);

        for (std::size_t idx = 0; idx < size; ++idx) {
            answer -= grad[idx] * (yPoint[idx] - xPoint[idx]);
        }
        return answer;
    }
private:
    /// @brief Object to evaluate prox-function d(x).
    std::shared_ptr<const IOrderOneOracle<ValueX>> xOracle;
    /// @brief Object to evaluate prox-function d(y).
    std::shared_ptr<const IOrderOneOracle<ValueY>> yOracle;
};

/**
 * @brief Interface for constraint descriptor entity. Defines the projection sub-space and at
 * the same time optimization domain.
 */
class IQDescriptor {
public:
    /**
     * @brief Forms constraints from variables. Pure virtual function.
     * 
     * @param variables The variables to use for making constraints.
     * @return Vector with constraints.
     */
    virtual std::vector<CppAD::AD<double>> makeConstraints(
            const std::vector<CppAD::AD<double>>& variables) const = 0;
    /**
     * @brief Builds bounds for constraint expressions generated by makeConstraints.
     * Pure virtual function.
     * 
     * @param glower Lower bounds for constraints.
     * @param gupper Upper bounds for constraints.
     */
    virtual void initConstraintsLR(
            std::vector<double>* glower, std::vector<double>* gupper) const = 0;
    /**
     * @brief Builds bounds for variables. Pure virtual function.
     * 
     * @param vlower Lower bounds for variables.
     * @param vupper Upper bounds for variables.
     */
    virtual void initBoundsLR(
            std::vector<double>* vlower, std::vector<double>* vupper) const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~IQDescriptor() {}
};

/**
 * @brief Projector interface. Projector is an entity to project point to problem domain.
 */
class IQProjector {
public:
    /**
     * @brief Projects to problem domain. Pure virtual function.
     * 
     * @param point The point to project.
     * @return The projected point.
     */
    virtual std::vector<double> project (std::vector<double> point) const = 0;
    virtual ~IQProjector() {};

};

/**
 * @brief Bregman Mapping Interface. Bregman Mapping is defined by the expression
 * B_M(x) = argmin_{y \in Q} PsiM(x, y) and is described at slide 14 of lecture 2 in 
 * Yuri Nesterov HSE optimization course.  
 */
class IBregmanMapping {
public:
    /**
     * @brief Computes Bregman mapping in point. Pure virtual function.
     * 
     * @param point Point x to compute Bregman mapping at.
     * @param valueM The value of M to parametrize PsiM function.
     * @return The point at which the PsiM minimum is achieved.
     */
    virtual std::vector<double> compute(std::vector<double> point, double valueM) const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~IBregmanMapping() {};
};

/**
 * @brief Interface to PsiM function. PsiM function is defined by the expression
 * PsiM(x, y) = f(x) + scalar_product(gradient(f(x)), y - x) + M * xi(x, y) + Psi(y),
 * where Psi(y) is regularizer and xi(x, y) is Bregman distance. The function PsiM
 * is described on slide 14 at lecture 2 in Yuri Nesterov HSE optimization course.
 * 
 * @tparam TypeX The type for computing x-dependent expressions.
 * @tparam TypeY The type for computing y-dependent expressions.
 */
template<typename TypeX, typename TypeY>
class IPsiM {
public:
    /**
     * @brief Get the PsiM value or expression (depending on template parameters).
     * Pure virtual function.
     * 
     * @param xPoint Point x for PsiM.
     * @param yPoint Point y for PsiM.
     * @param valueM The value of M in PsiM expression.
     * @param trgGrad Gradient of the target function.
     * @return The value or expression of PsiM function.
     */
    virtual TypeY getValue(
            const std::vector<TypeX>& xPoint,
            const std::vector<TypeY>& yPoint,
            double valueM,
            std::vector<TypeX>* trgGrad=nullptr) const = 0;
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~IPsiM() {};
};

} // namespace gm
