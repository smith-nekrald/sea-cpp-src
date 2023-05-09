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

const double INF = std::numeric_limits<double>::max();
const double FLOOR_EPS = 1e-3;
const double VARIABLES_INIT = 1e-4;
const double LOG_EPS = 1e-50;
const double HIRE_EPS = 1e-2;
const double DEMAND_EPS = 1e-2;
const double PRICE_EPS = 1e-2;


void IpoptBackend::initializeSuppliedAllotments(DecisionManagerPtr decisionManagerToWrite,
        vector<double>* vlower, vector<double>* vupper, vector<double>* variables) {
    const auto& input =  config.inputManager->getConstData();
    const auto& indexMap = indexManager->getConstData();
    for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        unsigned variableId = indexMap.allotmentToUIndex[idAllotment];
        auto* decision = decisionManagerToWrite->get();
        if (decision->allotmentAccepted[idAllotment]) {
            variables->at(variableId) = vlower->at(variableId) = vupper->at(variableId) = 1;
        } else {
            variables->at(variableId) = vlower->at(variableId) = vupper->at(variableId) = 0;
        }
    }
}

void IpoptBackend::setAlreadyMadeDecisions(
        TimeParameters timeParameters,
        DecisionManagerPtr decisionManagerToWrite,
        ConstActionManagerPtr currentActionManager,
        vector<double>* vlower, vector<double>* vupper, vector<double>* variables) {
    for (unsigned prevTime = 0; prevTime < timeParameters.timeEvent; ++prevTime) {
        // Restoring structures in case they were dumped.
        const auto& input = config.inputManager->getConstData();
        const auto& links = config.linksManager->getConstData();
        const auto& indexMap = indexManager->getConstData();
        const auto& event = input.events[prevTime];
        auto* decision = decisionManagerToWrite->get();
        if (event.type == EventType::cutoff) {
            const auto& arc = input.arcs[event.basedArc.value()];
            for (unsigned idItinerary : event.relatedItineraryIds) {
                // Processing Q_r
                unsigned takeQIndex = indexMap.idItineraryToQIndex[idItinerary];
                variables->at(takeQIndex) = vlower->at(takeQIndex)
                    = vupper->at(takeQIndex) = decision->nonEmptyContainersQ[idItinerary];
                // Processing Z_r
                unsigned emptyZIndex = indexMap.idItineraryToZIndex[idItinerary];
                variables->at(emptyZIndex) = vlower->at(emptyZIndex)
                    = vupper->at(emptyZIndex) = decision->emptyContainersZ[idItinerary];
                // Processing allotments
                for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                    unsigned takeIQIndex =
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
                    variables->at(takeIQIndex) = vlower->at(takeIQIndex)
                        = vupper->at(takeIQIndex) = targetValue;
                }
            }
            // tracking variables y_a^H
            unsigned hireIndex = indexMap.arcToHired[arc.id];
            vlower->at(hireIndex) = vupper->at(hireIndex)
                = variables->at(hireIndex) = decision->hiredY[arc.id];
        } else if (event.type == EventType::pricing) {
            for (auto idItinerary : event.relatedItineraryIds) {
                auto* action = &currentActionManager->getConstData();
                unsigned bookingAmount = action->bookingsB[prevTime][idItinerary];
                unsigned valueIndex = indexMap.timeItineraryToDemandIndex[prevTime][idItinerary];
                variables->at(valueIndex) = vupper->at(valueIndex)
                    = vlower->at(valueIndex) = static_cast<double>(bookingAmount);
            }
        } else if (event.type == EventType::arrival) {
            const auto& arc = input.arcs[event.basedArc.value()];
            const auto& node = input.nodes[arc.toNode];
            const auto& port = input.ports[node.portId];
            unsigned variableIndex = indexMap.timeToSIndex[event.relativeTime];
            variables->at(variableIndex) = vlower->at(variableIndex) = vupper->at(variableIndex) =
                static_cast<double>(decision->offHiredInPortS[prevTime][port.id]);
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


void IpoptBackend::recalculate(
        TimeParameters timeParameters,
        DecisionManagerPtr decisionManagerToWrite,
        ConstActionManagerPtr currentActionManager,
        bool writeDecision,
        vector<bool>* allotmentDecisionResult,
        double* objectiveEstimation,
        DualVariables* duals) {

    auto& logger = logging::getBackendSubLogger(BackendType::IPOPT);
    logger.debug("IpoptBackend::recalcuate is called.");
    logger.debugStream() << "TimeParameters:: allotments supplied "
        <<  timeParameters.allotmentsSupplied;

    assert(timeParameters.timeEvent < config.inputManager->getConstData().events.size());

    vector<double> vlower, vupper, glower, gupper, variables;
    initConstraintsLR(&glower, &gupper);
    initBoundsLR(&vlower, &vupper);

    if (canInitVariables) {
        assert(indexManager->getConstData().variableCount == lastVariables.size());
        variables = lastVariables;
    } else {
        size_t variableCount = indexManager->getConstData().variableCount;
        variables.assign(variableCount, VARIABLES_INIT);
        for (size_t idx = 0; idx  < variableCount; ++idx) {
            if (vlower[idx] != -INF) { variables[idx] = vlower[idx]; }
            if (vupper[idx] != INF) { variables[idx] = vupper[idx]; }
            if (vlower[idx] != -INF && vupper[idx] != INF) {
                variables[idx] = (vlower[idx] + vupper[idx]) / 2.;
            }
            if (vlower[idx] == -INF && vupper[idx] == INF) {
                variables[idx] = VARIABLES_INIT;
            };
        }
    }

    if (timeParameters.allotmentsSupplied) {
        initializeSuppliedAllotments(decisionManagerToWrite, &vlower, &vupper, &variables);
    }
    setAlreadyMadeDecisions(timeParameters, decisionManagerToWrite, currentActionManager,
            &vlower, &vupper, &variables);

    // Creating problem and calling IPOPT.
    OptimizationConfig problemConfig;
    problemConfig.indexManager = indexManager;
    problemConfig.inputManager = config.inputManager;
    problemConfig.linksManager = config.linksManager;
    problemConfig.actionManager = currentActionManager;
    problemConfig.decisionManager = decisionManagerToWrite;
    problemConfig.useEnhancedVersion = config.useEnhancedVersion;
    problemConfig.updateTime = timeParameters.timeEvent;
    problemConfig.utilizationRatio = utilizationRatio;
    OptimizationProblem problem(problemConfig);

    logConstraints(vlower, vupper, glower, gupper, variables,
            indexManager->getConstData());

    std::string options = makeOptionsFromConfig(config);
    logOptions(options);

    CppAD::ipopt::solve_result<vector<double>> solution;
    CppAD::ipopt::solve<std::vector<double>, OptimizationProblem>(
          options, variables, vlower, vupper, glower, gupper, problem, solution
    );
    assert(variables.size() == solution.x.size());
    variables = solution.x;

    logger.debugStream() << "Expected ipopt-objective to get is: "
        << -solution.solve_result::obj_value;
    if (!timeParameters.allotmentsSupplied) {
        logger.debugStream() << "Allotments are not supplied. Ipopt will decide on its own.";

        // Restoring structures in case they were dumped.
        const auto& input = config.inputManager->getConstData();
        const auto& indexMap = indexManager->getConstData();

        const auto& solutionValues = variables;
        vector<bool> acceptedAllotments(input.allotments.size(), false);
        logger.debugStream() << "input.allotments.size() = " <<  input.allotments.size();

        for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
            unsigned variableId = indexMap.allotmentToUIndex[idAllotment];
            double value = solutionValues[variableId];
            logger.debugStream() << "To be floored: " <<  "allotment_id "
                << idAllotment << " value " << value;
            assert(value + FLOOR_EPS >= 0);
            unsigned integer = floor(value + FLOOR_EPS);
            assert(integer == 0 || integer == 1);
            acceptedAllotments[idAllotment] = integer;
            variables[variableId] = vlower[variableId] = vupper[variableId] = integer;
        }

        if (writeDecision) {
            auto* decision = decisionManagerToWrite->get();
            decision->allotmentAccepted = acceptedAllotments;
        }

        if (allotmentDecisionResult != nullptr) {
            auto stream = logger.getStream(log4cpp::Priority::DEBUG);
            stream << "Assigning decision to accepted allotments." << "\n";
            *allotmentDecisionResult = acceptedAllotments;
            for (auto v : *allotmentDecisionResult) {
                stream << v << " ";
            }
        }
    }
    if (config.saveVariables) {
        canInitVariables = true;
        lastVariables = variables;
    }

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
                double value = solution.lambda[place - 1];
                duals->lambdaVariables[index.arcToLambdaIndex[arc.id]] = value;
            }
        }
    }

    // Restoring structures in case they were dumped.
    if (writeDecision) {
        const auto& input = config.inputManager->getConstData();
        const auto& links = config.linksManager->getConstData();
        const auto& indexMap = indexManager->getConstData();
        auto* decision = decisionManagerToWrite->get();

        // Writing results to decision.
        const auto& solutionValues = solution.x;
        for (unsigned nextTime = timeParameters.timeEvent;
                nextTime < input.events.size(); ++nextTime) {
            for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
                decision->offHiredInPortS[nextTime][idPort] = 0;
            }
            const auto& event = input.events[nextTime];
            if (event.type == EventType::cutoff) {

                const auto& arc = input.arcs[event.basedArc.value()];
                for (unsigned idItinerary : event.relatedItineraryIds) {
                    // Processing Q_r
                    unsigned takeQIndex = indexMap.idItineraryToQIndex[idItinerary];
                    double valueQ = floor(solution.x[takeQIndex] + FLOOR_EPS);
                    assert(valueQ >= 0);
                    decision->nonEmptyContainersQ[idItinerary] = unsigned(valueQ);

                    // Processing Z_r (setting decision and bounds)
                    unsigned emptyZIndex = indexMap.idItineraryToZIndex[idItinerary];
                    double valueZ = floor(solution.x[emptyZIndex] + FLOOR_EPS);
                    assert(valueZ >= 0);
                    decision->emptyContainersZ[idItinerary] = unsigned(valueZ);

                    // Processing allotments
                    for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                        unsigned takeIQIndex = indexMap.allotmentItineraryToQIndex[
                            idAllotment][idItinerary];
                        double valueIQ = floor(solution.x[takeIQIndex] + FLOOR_EPS);
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
                if (solution.x[hireIndex] > HIRE_EPS) {
                    decision->hiredY[arc.id] = ceil(solution.x[hireIndex]);
                }

            } else if (event.type == EventType::pricing) {
                for (unsigned index = 0; index < event.relatedItineraryIds.size(); ++index) {
                    unsigned itineraryId = event.relatedItineraryIds[index];
                    unsigned variableId = indexMap.timeItineraryToDemandIndex[
                        nextTime][itineraryId];
                    double realDemandValue = solutionValues[variableId] + DEMAND_EPS;
                    assert(realDemandValue >= 0.);
                    const auto& demand = event.demands[index];
                    double price = INF;
                    if (demand.type == Demand::Type::exponential) {
                        price = log(demand.scale / realDemandValue + LOG_EPS) / demand.sensitivity;
                    } else if (demand.type == Demand::Type::linear) {
                        price = (realDemandValue - demand.additive) / demand.multiplicative;
                    } else {
                        throw std::logic_error("Unknown Demand Type");
                    }
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
                double offhired = floor(solution.x[variableIndex] + FLOOR_EPS);
                assert(offhired >= 0);
                decision->offHiredInPortS[nextTime][port.id] = offhired;
            } else {
                throw std::logic_error("Event type is not supported");
            }
        }
    }
    if (objectiveEstimation != nullptr) {
        *objectiveEstimation = -solution.solve_result::obj_value;
    }

}

} // namespace backend
} // namespace sea
