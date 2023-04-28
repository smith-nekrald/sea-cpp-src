#include "ufgm_optimizer.h"
#include "ufgm_problem.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>
#include <numeric>

#include <cppad/ipopt/solve.hpp>

namespace gm {

using std::vector;
using std::string;
using CppAD::AD;
using std::size_t;

UFGMOptimizer::UFGMOptimizer(
        std::shared_ptr<DoubleFunction> target,
        std::shared_ptr<DoubleFunction> regularizer,
        std::shared_ptr<DoubleFunction> prox,
        std::shared_ptr<CppADFunction> adTarget,
        std::shared_ptr<CppADFunction> adRegularizer,
        std::shared_ptr<CppADFunction> adProx,
        std::shared_ptr<IQDescriptor> qDescriptor,
        size_t maxIter,
        double eps,
        double L0,
        bool keepStory)
    : IGradientOptimizer(keepStory, "UFGM")
    , target(target)
    , regularizer(regularizer)
    , prox(prox)
    , adTarget(adTarget)
    , adRegularizer(adRegularizer)
    , adProx(adProx)
    , qDescriptor(qDescriptor)
    , maxIter(maxIter)
    , eps(eps)
    , L0(L0) {
}

vector<double> UFGMOptimizer::optimize(
        const vector<double>& initial) const {
    story.clear();

    vector<double> xk = initial, yk = initial, vk = initial;
    size_t problemSize = xk.size();
    const size_t MAX_POW = 256;

    vector<double> coeffAns(problemSize);
    double coeffReg = 0,
           coeffFree = 0;

    double Lk = L0, Ak = 0, ak = 0, tau_k = 0;

    vector<double> xBest = initial;
    double valueBest = target->getValue(xBest);

    bool findPow = true;
    size_t pow = 0;
    for (size_t iter_id = 0; iter_id < maxIter && findPow; ++iter_id) {

        std::cout << "UFGMOptimizer : iteration = " << iter_id + 1 << " ";
        std::cout << "valueBest = " << valueBest << " last_pow = " << pow <<  " ";
        std::cout << "Ak = " << Ak << " Lk = " << Lk << " tau_k = " << tau_k << std::endl;
        if (Ak > 1e19) break;

        double equ_a = 1, equ_b = -1. /Lk, equ_c = -Ak / Lk;
        vk = compute_argmin_psi(coeffAns, coeffReg, coeffFree, initial, vk);

        double pow2 = 1.;
        findPow = false;
        for (pow = 0; pow < MAX_POW && !findPow; ++pow) {
            bool ok = true;
            auto discr = equ_b * equ_b - 4. * equ_a * equ_c;
            if (discr < 0) {
                ok = false;
            }
            vector<double> ak_cands = {};
            if (ok) {
                auto ak1 = (-equ_b + std::sqrt(discr)) / (2. * equ_a);
                auto ak2 = (-equ_b - std::sqrt(discr)) / (2. * equ_a);
                for (auto& item : {ak1, ak2}) {
                    if (item > 0) {
                        ak_cands.push_back(item);
                    }
                }
            }
            for (const auto& cand_ak : ak_cands) {
                auto cand_Ak = Ak + cand_ak;
                auto cand_tauk = cand_ak / cand_Ak;

                vector<double> cand_xk(problemSize, 0);
                for (size_t idx = 0; idx < problemSize; ++idx) {
                    cand_xk[idx] = cand_tauk * vk[idx] + (1. - cand_tauk) * yk[idx];
                }

                auto f_grad = target->getSubgradient(cand_xk);
                vector<double> cand_xk_hat = compute_argmin_bregman(
                        f_grad, cand_ak, vk, xk);

                vector<double> cand_yk(problemSize, 0);
                for (size_t idx = 0; idx < problemSize; ++idx) {
                    cand_yk[idx] = cand_xk_hat[idx] * cand_tauk + (1. - cand_tauk) * yk[idx];
                }


                double scalarProduct = 0, diffSqrNorm = 0;
                for (size_t idx = 0; idx < problemSize; ++idx) {
                    scalarProduct += f_grad[idx] * (cand_yk[idx] - cand_xk[idx]);
                    diffSqrNorm += (cand_yk[idx] - cand_xk[idx])
                        * (cand_yk[idx] - cand_xk[idx]);
                }


                if (target->getValue(cand_yk) <= target->getValue(cand_xk) +
                        scalarProduct + pow2 / 2. * Lk * diffSqrNorm + eps / 2 * cand_tauk) {

                    // Candidates fits the condition. Accept candidate.
                    xk = cand_xk;
                    yk = cand_yk;
                    ak = cand_ak;
                    tau_k = cand_tauk;

                    for (const auto& item : {xk, yk, vk, cand_xk_hat}) {
                        auto thisValue = target->getValue(item);
                        if (thisValue < valueBest) {
                            valueBest = thisValue;
                            xBest = item;
                        }
                    }
                    Ak += ak;
                    Lk = Lk * pow2 / 2.;

                    // Recalculate coefficients for vk compuation.
                    auto fk = target->getValue(xk);
                    auto grad_fk = target->getSubgradient(xk);
                    coeffFree += ak * fk;
                    coeffFree -= ak * std::inner_product(
                        grad_fk.begin(), grad_fk.end(), xk.begin(), 0);
                    coeffReg += ak;
                    for (size_t idx = 0; idx < coeffAns.size(); ++idx) {
                        coeffAns[idx] += ak * grad_fk[idx];
                    }
                    // Leave cycle and power search.
                    findPow = true;
                    if (keepStory) {
                        story["current_best"].push_back(valueBest);
                        auto ykValue = target->getValue(yk);
                        story["yk_value"].push_back(ykValue);
                        story["ak_value"].push_back(Ak);
                        story["Lk_value"].push_back(Lk);
                        story["yk_targ_plus_reg"].push_back(ykValue + regularizer->getValue(yk));
                        story["tau_k_value"].push_back(tau_k);
                    }
                    break;
                }
            }
            // Recomputation of coefficients in quadratic equation.
            equ_b /= 2;
            equ_c /= 2;
            pow2 *= 2;
        }
    }
    return xBest;
};

void UFGMOptimizer::setCppADToValues(const vector<double>& values,
        vector<AD<double>>* variablesPtr) const {
    assert(variablesPtr != nullptr);
    auto& variables = *variablesPtr;
    assert(variables.size() == values.size());
    size_t size = variables.size();
    for (size_t idx = 0; idx < size; ++idx) {
        variables[idx] = values[idx];
    }
}

vector<double> UFGMOptimizer::compute_argmin_bregman(
        const std::vector<double>& grad_f_xk,
        double ak,
        const std::vector<double>& bregmanPivot,
        const std::vector<double>& init) const {

    vector<double> vlower, vupper, glower, gupper, variables = init;
    qDescriptor->initConstraintsLR(&glower, &gupper);
    qDescriptor->initBoundsLR(&vlower, &vupper);

    BregmanOptimizationProblem problem(
            grad_f_xk, ak, bregmanPivot, qDescriptor,
            regularizer, prox,
            adRegularizer, adProx);

    string options = makeIpoptOptions();
    CppAD::ipopt::solve_result<vector<double>> solution; // scaled
    CppAD::ipopt::solve<std::vector<double>, BregmanOptimizationProblem>(
        options, variables, vlower, vupper, glower, gupper, problem, solution
    );
    variables = solution.x;
    return variables;
}

vector<double> UFGMOptimizer::compute_argmin_psi(
        const std::vector<double>& coeffAns,
        double coeffReg,
        double coeffFree,
        const std::vector<double>& x0,
        const std::vector<double>& init) const {
    vector<double> vlower, vupper, glower, gupper, variables = init;
    qDescriptor->initConstraintsLR(&glower, &gupper);
    qDescriptor->initBoundsLR(&vlower, &vupper);

    assert(vlower.size() == vupper.size());
    assert(glower.size() == gupper.size());
    assert(init.size() == vlower.size());

    ArgminOptimizationProblem problem(coeffAns, x0, coeffReg, coeffFree,
            qDescriptor, regularizer, prox, adRegularizer, adProx);
    string options = makeIpoptOptions();
    CppAD::ipopt::solve_result<vector<double>> solution;
    CppAD::ipopt::solve<std::vector<double>, ArgminOptimizationProblem>(
        options, variables, vlower, vupper, glower, gupper, problem, solution
    );
    variables = solution.x;
    return variables;
}

string UFGMOptimizer::makeIpoptOptions() const {
    std::string options = "";
    options += "Sparse true reverse \n";
    options += "String linear_solver ma57 \n";
    options += "Integer max_iter     3000 \n"; // default 3000
    options += "Numeric tol          1e-4 \n"; // default 1e-8
    options += "Integer print_level  0   \n"; // default is 5
    options += "String  sb           yes \n";
    return options;
}

} // namespace gm
