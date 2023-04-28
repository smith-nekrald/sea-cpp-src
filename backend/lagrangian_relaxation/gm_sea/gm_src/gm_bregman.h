#pragma once


#include "gm_interfaces.h"


namespace gm {

template<typename TypeX, typename TypeY>
class PsiM : public IPsiM<TypeX, TypeY> {
public:
    PsiM(std::shared_ptr<const IOrderOneOracle<TypeX>> aTarget,
        std::shared_ptr<const IOrderOneOracle<TypeY>> aRegularizer,
        std::shared_ptr<const BregmanDistance<TypeX, TypeY>> aBregman)
            : target(aTarget)
            , regularizer(aRegularizer)
            , bregman(aBregman) {

    }
    virtual TypeY getValue(
            const std::vector<TypeX>& xPoint,
            const std::vector<TypeY>& yPoint,
            double M,
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
        return valF + scalarProd + M * bregmanDist + regVal;
    }
    virtual ~PsiM() {};
private:
    std::shared_ptr<const IOrderOneOracle<TypeX>> target;
    std::shared_ptr<const IOrderOneOracle<TypeY>> regularizer;
    std::shared_ptr<const BregmanDistance<TypeX, TypeY>> bregman;
};


class OptimizationProblemBregmanMapping {
public:
    typedef std::vector<CppAD::AD<double>> ADvector;
public:
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
        , M(anM) {

        gradInPoint = target->getSubgradient(point);
    };
    void operator()(ADvector& functions, const ADvector& variables) {
        auto constraints = qDescriptor->makeConstraints(variables);
        functions.resize(constraints.size() + 1);

        for (size_t idx = 0; idx < constraints.size(); ++idx) {
            functions[idx + 1] = constraints[idx];
        }
        functions[0] = PsiM<double, CppAD::AD<double>>(target, regularizer, bregman).getValue(
                point, variables, M, &gradInPoint);
    }

private:
    std::shared_ptr<const IQDescriptor> qDescriptor;
    std::shared_ptr<const DoubleFunction> target;
    std::shared_ptr<const CppADFunction> regularizer;
    std::shared_ptr<const BregmanDistance<double, CppAD::AD<double>>> bregman;

    std::vector<double> point;
    std::vector<double> gradInPoint;
    double M;
};


class BregmanMapping : public IBregmanMapping {
public:
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
            bregman = std::make_shared<
                BregmanDistance<double, CppAD::AD<double>>>(prox, adProx);
    }

    virtual std::vector<double> compute(std::vector<double> point, double M) const override {

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
                qDescriptor,target, regularizer, bregman, point, M);
        CppAD::ipopt::solve_result<std::vector<double>> solution;
        CppAD::ipopt::solve<std::vector<double>, OptimizationProblemBregmanMapping>(
            options, variables,
            vlower, vupper,
            glower, gupper,
            problem, solution
        );
        variables = solution.x;

        return variables;

    };
private:
    std::shared_ptr<const DoubleFunction> target;
    std::shared_ptr<const CppADFunction> regularizer;
    std::shared_ptr<const DoubleFunction> prox;
    std::shared_ptr<const CppADFunction> adProx;
    std::shared_ptr<const IQDescriptor> qDescriptor;
    std::shared_ptr<BregmanDistance<double, CppAD::AD<double>>> bregman;

};

} // namespace gm
