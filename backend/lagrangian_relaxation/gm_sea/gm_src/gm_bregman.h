/**
 * @file gm_bregman.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implementation for Bregman Mapping and intermediate classes, including PsiM.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "gm_interfaces.h"

namespace gm {

/**
 * @brief Implements PsiM function. PsiM function is defined by the expression
 * PsiM(x, y) = f(x) + scalar_product(gradient(f(x)), y - x) + M * xi(x, y) + Psi(y),
 * where Psi(y) is regularizer and xi(x, y) is Bregman distance. The function PsiM
 * is described on slide 14 at lecture 2 in Yuri Nesterov HSE optimization course.
 * 
 * @tparam TypeX The type for computing x-dependent expressions.
 * @tparam TypeY The type for computing y-dependent expressions.
 */
template<typename TypeX, typename TypeY>
class PsiM : public IPsiM<TypeX, TypeY> {
public:
    /**
     * @brief PsiM constructor.
     * 
     * @param aTarget Object to compute and differentiate target function.
     * @param aRegularizer Object to compute and differentiate regularizer.
     * @param aBregman Object to compute Bregman Distance.
     */
    PsiM(std::shared_ptr<const IOrderOneOracle<TypeX>> aTarget,
        std::shared_ptr<const IOrderOneOracle<TypeY>> aRegularizer,
        std::shared_ptr<const BregmanDistance<TypeX, TypeY>> aBregman)
            : target(aTarget)
            , regularizer(aRegularizer)
            , bregman(aBregman) {
    }
    /**
     * @brief Get the PsiM value or expression (depending on template parameters).
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
            std::vector<TypeX>* targetGrad=nullptr) const override {
        std::vector<TypeX> gradF;
        if (targetGrad != nullptr) {
            gradF = *targetGrad;
        } else {
            gradF = target->getSubgradient(xPoint);
        }
        auto regVal = regularizer->getValue(yPoint);
        auto bregmanDist = bregman->getDistance(xPoint, yPoint);
        TypeY scalarProd = 0;
        for (size_t idx = 0; idx < gradF.size(); ++idx) {
            scalarProd += gradF[idx] * (yPoint[idx] - xPoint[idx]);
        }
        auto valF = target->getValue(xPoint);
        return valF + scalarProd + valueM * bregmanDist + regVal;
    }
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~PsiM() {};

private:
    /// @brief Object to compute and differentiate target function.
    std::shared_ptr<const IOrderOneOracle<TypeX>> target;
    /// @brief Object to compute and differentiate regularizer.
    std::shared_ptr<const IOrderOneOracle<TypeY>> regularizer;
    /// @brief Object to compute Bregman Distance.
    std::shared_ptr<const BregmanDistance<TypeX, TypeY>> bregman;
};


/**
 * @brief Entity to form objective and constraints for computing Bregman Mapping.
 */
class OptimizationProblemBregmanMapping {
public:
    /// @brief Simplified notation for CppAD vector.
    typedef std::vector<CppAD::AD<double>> ADvector;

public:
    /**
     * @brief Constructs optimization problem.
     * 
     * @param aQDescriptor Object for expressing constraints and bounds.
     * @param aTarget Object to compute and differentiate target function.
     * @param aRegularizer Object to compute and differentiate regularizer.
     * @param aBregman Object for computing and expressing Bregman distance.
     * @param aPoint Point to compute Bregman mapping at.
     * @param anM The parameter M in PsiM function.
     */
    OptimizationProblemBregmanMapping(std::shared_ptr<const IQDescriptor> aQDescriptor,
                std::shared_ptr<const DoubleFunction> aTarget,
                std::shared_ptr<const CppADFunction> aRegularizer,
                std::shared_ptr<const BregmanDistance<double, CppAD::AD<double>>> aBregman,
                std::vector<double> aPoint,
                double anM)
            : qDescriptor(aQDescriptor)
            , target(aTarget)
            , regularizer(aRegularizer)
            , bregman(aBregman)
            , point(aPoint)
            , valueM(anM) {
        gradInPoint = target->getSubgradient(point);
    };
    /**
     * @brief Forms objective and constraints.
     * 
     * @param functions Place to write objective (functions[0]) and constraints (functions[1:]).
     * @param variables Independent variables to form objective and constraints from.
     */
    void operator()(ADvector& functions, const ADvector& variables) {
        auto constraints = qDescriptor->makeConstraints(variables);
        functions.resize(constraints.size() + 1);

        for (size_t idx = 0; idx < constraints.size(); ++idx) {
            functions[idx + 1] = constraints[idx];
        }
        functions[0] = PsiM<double, CppAD::AD<double>>(target, regularizer, bregman).getValue(
                point, variables, valueM, &gradInPoint);
    }

private:
    /// @brief Object for expressing constraints and bounds.
    std::shared_ptr<const IQDescriptor> qDescriptor;
    /// @brief Object for computing and differentiating the target function.
    std::shared_ptr<const DoubleFunction> target;
    /// @brief Object for expressing and differentiating the regularizer.
    std::shared_ptr<const CppADFunction> regularizer;
    /// @brief Object for expressing Bregman Distance.
    std::shared_ptr<const BregmanDistance<double, CppAD::AD<double>>> bregman;

    /// @brief The point to compute Bregman Mapping at.
    std::vector<double> point;
    /// @brief Target gradient in point.
    std::vector<double> gradInPoint;
    /// @brief Value of parameter M in PsiM.
    double valueM;
};

/**
 * @brief Implements Bregman Mapping. Bregman Mapping is defined by the expression
 * B_M(x) = argmin_{y \in Q} PsiM(x, y) and is described at slide 14 of lecture 2 in 
 * Yuri Nesterov HSE optimization course.  
 */
class BregmanMapping : public IBregmanMapping {
public:
    /**
     * @brief Constructs Bregman Mapping.
     * 
     * @param aTarget Object for computing function value and gradient.
     * @param aRegularizer Object for expressing regularizer value and gradient in CppAD form.
     * @param aProx Object for computing prox-function value and gradient.
     * @param anADProx Object for expressing prox-function value and gradient in CppAD form.
     * @param aQDescriptor Object for building constraints and variable bounds.
     */
    BregmanMapping(
                std::shared_ptr<const DoubleFunction> aTarget,
                std::shared_ptr<const CppADFunction> aRegularizer,
                std::shared_ptr<const DoubleFunction> aProx,
                std::shared_ptr<const CppADFunction> anADProx,
                std::shared_ptr<const IQDescriptor> aQDescriptor)
            : target(aTarget)
            , regularizer(aRegularizer)
            , prox(aProx)
            , adProx(anADProx)
            , qDescriptor(aQDescriptor) {
        bregman = std::make_shared<BregmanDistance<double, CppAD::AD<double>>>(prox, adProx);
    }
    /**
     * @brief Computes Bregman mapping in point. Pure virtual function.
     * 
     * @param point Point x to compute Bregman mapping at.
     * @param valueM The value of M to parametrize PsiM function.
     * @return The point at which the PsiM minimum is achieved.
     */
    virtual std::vector<double> compute(std::vector<double> point, double valueM) const override {
        std::string options = "";
        options += "Sparse true reverse \n";
        options += "String linear_solver ma57 \n";
        options += "Integer max_iter     3000 \n"; // default 3000
        options += "Numeric tol          1e-4 \n"; // default 1e-8
        options += "Integer print_level  0   \n"; // default is 5
        options += "String  sb           yes \n";

        std::vector<double> vlower, vupper, glower, gupper, variables = point;

        qDescriptor->initConstraintsLR(&glower, &gupper);
        qDescriptor->initBoundsLR(&vlower, &vupper);

        OptimizationProblemBregmanMapping problem(
            qDescriptor,target, regularizer, bregman, point, valueM);
        CppAD::ipopt::solve_result<std::vector<double>> solution;
        CppAD::ipopt::solve<std::vector<double>, OptimizationProblemBregmanMapping>(
            options, variables, vlower, vupper, glower, gupper, problem, solution);
        variables = solution.x;

        return variables;
    };
    /**
     * @brief Virtual destructor to ensure correct C++ polymorphism.
     */
    virtual ~BregmanMapping() {};

private:
    /// @brief Object for computing optimization function value and gradient.
    std::shared_ptr<const DoubleFunction> target;
    /// @brief Object for expressing regularizer value and gradient in CppAD form.
    std::shared_ptr<const CppADFunction> regularizer;
    /// @brief Object for computing prox-function value and gradient.
    std::shared_ptr<const DoubleFunction> prox;
    /// @brief Object for expressing prox-function value and gradient in CppAD form.
    std::shared_ptr<const CppADFunction> adProx;
    /// @brief Object for building constraints and variable bounds.
    std::shared_ptr<const IQDescriptor> qDescriptor;
    /// @brief Object for expressing Bregman Distance in CppAD form.
    std::shared_ptr<BregmanDistance<double, CppAD::AD<double>>> bregman;
};

} // namespace gm
