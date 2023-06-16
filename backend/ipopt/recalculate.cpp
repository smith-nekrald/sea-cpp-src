// Implements IpoptBackend::recalculate and corresponding stages.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "ipopt_backend.h"
#include "optimization_problem.h"
#include "../lagrangian_relaxation/index.h"
#include "../../logging/logging.h"

#include <string>
#include <limits>
#include <memory>
#include <cassert>

namespace sea {
namespace backend {

using std::unordered_map;
using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;
using std::cout;
using std::endl;
using std::size_t;


void IpoptBackend::formDuals(const vector<double>& lambdas, DualVariables* duals) const {

    if (duals != nullptr) {
        const auto& input = config.inputManager->getConstData();
        const auto& links = config.linksManager->getConstData();
        LagrangianRelaxationIndex index;
        initLagrangianRelaxationIndex(links, input, &index);

        duals->lambdaVariables.assign(index.lambdaCount, 0);
        duals->muVariables.assign(index.muCount, 0);

        for (const auto& arc: input.arcs) {
            if (arc.type == ArcType::travel) {
                assert(duals->lambdaVariables.size());
                size_t place = indexManager->getConstData().arcCapacityConstraints[arc.id];
                double value = lambdas[place - 1];
                duals->lambdaVariables[index.arcToLambdaIndex[arc.id]] = value;
            }
        }
    }
}

void IpoptBackend::initializeSuppliedAllotments(ConstDecisionManagerPtr decisionManager,
        vector<double>* vlowerPtr, vector<double>* vupperPtr, vector<double>* variablesPtr) const {

    const auto& input =  config.inputManager->getConstData();
    const auto& indexMap = indexManager->getConstData();
    for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        unsigned variableId = indexMap.allotmentToUIndex[idAllotment];
        auto* decision = decisionManager->get();
        if (decision->allotmentAccepted[idAllotment]) {
            variablesPtr->at(variableId) = vlowerPtr->at(variableId)
                = vupperPtr->at(variableId) = 1;
        } else {
            variablesPtr->at(variableId) = vlowerPtr->at(variableId)
                = vupperPtr->at(variableId) = 0;
        }
    }
}

void IpoptBackend::setPreviouslyMadeDecisions(
        const TimeParameters& timeParameters,
        ConstDecisionManagerPtr decisionManager,
        ConstActionManagerPtr currentActionManager,
        vector<double>* vlowerPtr,
        vector<double>* vupperPtr,
        vector<double>* variablesPtr) const {

    for (unsigned prevTime = 0; prevTime < timeParameters.timeEvent; ++prevTime) {
        const auto& input = config.inputManager->getConstData();
        const auto& links = config.linksManager->getConstData();
        const auto& indexMap = indexManager->getConstData();
        const auto& event = input.events[prevTime];
        auto* decision = decisionManager->get();
        if (event.type == EventType::cutoff) {
            const auto& arc = input.arcs[event.basedArc.value()];
            for (unsigned idItinerary : event.relatedItineraryIds) {
                // Processing Q_r
                unsigned boardQIndex = indexMap.idItineraryToQIndex[idItinerary];
                variablesPtr->at(boardQIndex) = vlowerPtr->at(boardQIndex)
                    = vupperPtr->at(boardQIndex) = decision->nonEmptyContainersQ[idItinerary];
                // Processing Z_r
                unsigned emptyZIndex = indexMap.idItineraryToZIndex[idItinerary];
                variablesPtr->at(emptyZIndex) = vlowerPtr->at(emptyZIndex)
                    = vupperPtr->at(emptyZIndex) = decision->emptyContainersZ[idItinerary];
                // Processing allotments
                for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                    unsigned loadIQIndex =
                        indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
                    unsigned placeIndex =
                        links.allotmentItineraryToPlace.at(idAllotment).at(idItinerary);

                    assert(decision->allotmentContainersQ[idAllotment][placeIndex].first
                            == idItinerary);
                    double targetValue =
                        decision->allotmentContainersQ[idAllotment][placeIndex].second;
                    if (!decision->allotmentAccepted[idAllotment]) {
                        targetValue = 0;
                    }
                    variablesPtr->at(loadIQIndex) = vlowerPtr->at(loadIQIndex)
                        = vupperPtr->at(loadIQIndex) = targetValue;
                }
            }
            // tracking variables y_a^H
            unsigned hireIndex = indexMap.arcToHired[arc.id];
            vlowerPtr->at(hireIndex) = vupperPtr->at(hireIndex)
                = variablesPtr->at(hireIndex) = decision->hiredY[arc.id];
        } else if (event.type == EventType::pricing) {
            for (auto idItinerary : event.relatedItineraryIds) {
                auto* action = &currentActionManager->getConstData();
                unsigned bookingAmount = action->bookingsB[prevTime][idItinerary];
                unsigned valueIndex = indexMap.timeItineraryToDemandIndex[prevTime][idItinerary];
                variablesPtr->at(valueIndex) = vupperPtr->at(valueIndex)
                    = vlowerPtr->at(valueIndex) = static_cast<double>(bookingAmount);
            }
        } else if (event.type == EventType::arrival) {
            const auto& arc = input.arcs[event.basedArc.value()];
            const auto& node = input.nodes[arc.toNode];
            const auto& port = input.ports[node.portId];
            unsigned variableIndex = indexMap.timeToSIndex[event.relativeTime];
            variablesPtr->at(variableIndex) = vlowerPtr->at(variableIndex)
                = vupperPtr->at(variableIndex)
                = static_cast<double>(decision->offHiredInPortS[prevTime][port.id]);
            for (const auto& otherPort : input.ports) {
                if (otherPort.id != port.id
                        && decision->offHiredInPortS[prevTime][otherPort.id] != 0) {
                    throw std::logic_error(
                        std::string("Deterministic Algorithm currently does not support")
                        + std::string("offhiring in other places than arrival port!"));
                }
            }
        }
    }
}

void IpoptBackend::supplyAllotmentsFromSolution(
        const vector<double>& solutionValues,
        DecisionManagerPtr decisionManagerToWrite,
        bool writeToDecision,
        vector<bool>* allotmentsToSelect,
        vector<double>* vlowerPtr,
        vector<double>* vupperPtr,
        vector<double>* variablesPtr) const {

    // Restoring structures in case they were dumped.
    const auto& input = config.inputManager->getConstData();
    const auto& indexMap = indexManager->getConstData();

    vector<bool> acceptedAllotments(input.allotments.size(), false);

    // Decision is based on the threshold. Above threshold converts in accepted allotment,
    // below threshold converts in declined allotment.
    const double ALLOTMENT_THRESHOLD = 0.8;
    for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
            unsigned variableId = indexMap.allotmentToUIndex[idAllotment];
            double value = solutionValues[variableId];
            unsigned integer = (value > ALLOTMENT_THRESHOLD ? 1 : 0);
            assert(integer == 0 || integer == 1);
            acceptedAllotments[idAllotment] = integer;
            variablesPtr->at(variableId) = vlowerPtr->at(variableId)
                = vupperPtr->at(variableId) = integer;
    }


    if (writeToDecision) {
        auto* decision = decisionManagerToWrite->get();
        decision->allotmentAccepted = acceptedAllotments;
    }

    if (allotmentsToSelect != nullptr) {
        *allotmentsToSelect = acceptedAllotments;
        logSelectedAllotments(acceptedAllotments, BackendType::IPOPT);
    }
}

void IpoptBackend::initVariables(const vector<double>& vlower,
        const vector<double>& vupper,
        vector<double>* variablesPtr) const {

    if (canInitVariables) {
        assert(indexManager->getConstData().variableCount == lastVariables.size());
        *variablesPtr = lastVariables;
    } else {
        const double VARIABLES_INIT = 1e-4;
        const double INF = std::numeric_limits<double>::max();
        size_t variableCount = indexManager->getConstData().variableCount;
        variablesPtr->assign(variableCount, VARIABLES_INIT);
        for (size_t idx = 0; idx  < variableCount; ++idx) {
            if (vlower[idx] != -INF) { variablesPtr->at(idx) = vlower[idx]; }
            if (vupper[idx] != INF) { variablesPtr->at(idx) = vupper[idx]; }
            if (vlower[idx] != -INF && vupper[idx] != INF) {
                variablesPtr->at(idx) = (vlower[idx] + vupper[idx]) / 2.;
            }
            if (vlower[idx] == -INF && vupper[idx] == INF) {
                variablesPtr->at(idx) = VARIABLES_INIT;
            };
        }
    }
}

void IpoptBackend::recalculate(
        TimeParameters timeParameters,
        DecisionManagerPtr decisionManagerToWrite,
        ConstActionManagerPtr currentActionManager,
        bool writeToDecision,
        vector<bool>* allotmentsToSelect,
        double* objectiveEstimation,
        DualVariables* duals) {

    assert(timeParameters.timeEvent < config.inputManager->getConstData().events.size());

    vector<double> vlower, vupper, glower, gupper, variables;
    initConstraintsLR(&glower, &gupper);
    initBoundsLR(&vlower, &vupper);
    initVariables(vlower, vupper, &variables);

    // Initialize allotments from the values stored in the decision.
    if (timeParameters.allotmentsSupplied) {
        initializeSuppliedAllotments(decisionManagerToWrite, &vlower, &vupper, &variables);
    }

    // Supply the historic information and previously made decisions for the trajectory prefix.
    setPreviouslyMadeDecisions(timeParameters, decisionManagerToWrite, currentActionManager,
            &vlower, &vupper, &variables);

    // Creating problem and calling IPOPT.
    OptimizationConfig problemConfig;
    problemConfig.indexManager = indexManager;
    problemConfig.inputManager = config.inputManager;
    problemConfig.linksManager = config.linksManager;
    problemConfig.actionManager = currentActionManager;
    problemConfig.decisionManager = decisionManagerToWrite;
    problemConfig.updateTime = timeParameters.timeEvent;
    problemConfig.utilizationRatio = utilizationRatio;
    OptimizationProblem problem(problemConfig);

    logConstraints(vlower, vupper, glower, gupper, variables, indexManager->getConstData());

    std::string options = makeOptionsFromConfig(config);
    logOptions(options);

    CppAD::ipopt::solve_result<vector<double>> solution;
    CppAD::ipopt::solve<std::vector<double>, OptimizationProblem>(
          options, variables, vlower, vupper, glower, gupper, problem, solution
    );
    assert(variables.size() == solution.x.size());
    variables = solution.x;

    if (!timeParameters.allotmentsSupplied) {
        const vector<double> solutionValues = solution.x;
        supplyAllotmentsFromSolution(
            solutionValues, decisionManagerToWrite, writeToDecision,
            allotmentsToSelect, &vlower, &vupper, &variables);
    }
    if (config.saveVariables) {
        canInitVariables = true;
        lastVariables = variables;
    }

    // Export duals in case they are required, i.e. duals != nullptr.
    formDuals(solution.lambda, duals);

    if (writeToDecision) {
        writeSolutionToDecision(timeParameters, solution.x, decisionManagerToWrite);
    }

    // If objective estimation is requested, fill the corresponding pointer.
    if (objectiveEstimation != nullptr) {
        *objectiveEstimation = -solution.solve_result::obj_value;
    }

}

void IpoptBackend::writeSolutionToDecision(
        const TimeParameters& timeParameters,
        const vector<double>& solutionValues,
    DecisionManagerPtr decisionManagerToWrite) const {

    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();
    const auto& indexMap = indexManager->getConstData();
    auto* decision = decisionManagerToWrite->get();

    // Writing results to decision.
    for (unsigned nextTime = timeParameters.timeEvent;
            nextTime < input.events.size(); ++nextTime) {
        for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
            decision->offHiredInPortS[nextTime][idPort] = 0;
        }
        const auto& event = input.events[nextTime];
        if (event.type == EventType::cutoff) {

            const auto& arc = input.arcs[event.basedArc.value()];
            for (unsigned idItinerary : event.relatedItineraryIds) {
                const double FLOOR_EPS = 1e-3;

                // Processing Q_r
                unsigned boardQIndex = indexMap.idItineraryToQIndex[idItinerary];
                double valueQ = floor(solutionValues[boardQIndex] + FLOOR_EPS);
                assert(valueQ >= 0);
                decision->nonEmptyContainersQ[idItinerary] = static_cast<unsigned>(valueQ);

                // Processing Z_r (setting decision and bounds)
                unsigned emptyZIndex = indexMap.idItineraryToZIndex[idItinerary];
                double valueZ = floor(solutionValues[emptyZIndex] + FLOOR_EPS);
                assert(valueZ >= 0);
                decision->emptyContainersZ[idItinerary] = static_cast<unsigned>(valueZ);

                // Processing allotments
                for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                    unsigned loadIQIndex = indexMap.allotmentItineraryToQIndex[
                        idAllotment][idItinerary];
                    double valueIQ = floor(solutionValues[loadIQIndex] + FLOOR_EPS);
                    assert(valueIQ >= 0);
                    unsigned placeIndex = links.allotmentItineraryToPlace.at(
                            idAllotment).at(idItinerary);
                    assert(decision->allotmentContainersQ[
                            idAllotment][placeIndex].first == idItinerary);
                    if (!decision->allotmentAccepted[idAllotment]) {
                        valueIQ = 0;
                    }
                    decision->allotmentContainersQ[idAllotment][placeIndex].second = valueIQ;
                }
            }

            // tracking variables y_a^H
            unsigned hireIndex = indexMap.arcToHired[arc.id];
            const double HIRE_EPS = 1e-2;
            if (solutionValues[hireIndex] > HIRE_EPS) {
                decision->hiredY[arc.id] = ceil(solutionValues[hireIndex]);
            }
        } else if (event.type == EventType::pricing) {
            for (unsigned index = 0; index < event.relatedItineraryIds.size(); ++index) {
                unsigned itineraryId = event.relatedItineraryIds[index];
                unsigned variableId = indexMap.timeItineraryToDemandIndex[
                    nextTime][itineraryId];
                const double DEMAND_EPS = 1e-2;
                double realDemandValue = solutionValues[variableId] + DEMAND_EPS;
                assert(realDemandValue >= 0.);
                const auto& demand = event.demands[index];
                double price = std::numeric_limits<double>::max();
                if (demand.type == Demand::Type::exponential) {
                    const double LOG_EPS = 1e-50;
                    price = log(demand.scale / realDemandValue + LOG_EPS) / demand.sensitivity;
                } else if (demand.type == Demand::Type::linear) {
                    price = (realDemandValue - demand.additive) / demand.multiplicative;
                } else {
                    throw std::logic_error("Unknown Demand Type");
                }
                const double PRICE_EPS = 1e-8;
                price += PRICE_EPS;
                assert(price >= 0.);
                auto& target = decision->prices[event.relativeTime][index];
                assert(target.first == itineraryId);
                target.second = price;
            }
        } else if (event.type == EventType::arrival) {
            const auto& arc = input.arcs[event.basedArc.value()];
            const auto& node = input.nodes[arc.toNode];
            const auto& port = input.ports[node.portId];
            unsigned variableIndex = indexMap.timeToSIndex[event.relativeTime];
            const double FLOOR_EPS = 1e-3;
            double offhired = floor(solutionValues[variableIndex] + FLOOR_EPS);
            assert(offhired >= 0);
            decision->offHiredInPortS[nextTime][port.id] = offhired;
        } else {
            throw std::logic_error("Event type is not supported");
        }
    }
}

} // namespace backend
} // namespace sea
