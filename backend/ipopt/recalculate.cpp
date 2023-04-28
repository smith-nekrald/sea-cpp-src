#include "ipopt_backend.h"
#include "optimization_problem.h"
#include "../lagrangian_relaxation/index.h"
#include "../../logging/logging.h"

#include <string>
#include <limits>
#include <memory>

namespace sea {
namespace backend {

using std::unordered_map;
using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;
using std::cout;
using std::endl;
using std::size_t;

const double INF = std::numeric_limits<double>::max();

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
    logger.debugStream() << "TimeParameters:: allotments supplied " <<  timeParameters.allotmentsSupplied;

    assert(timeParameters.timeEvent <
            config.inputManager->getConstData().events.size());

    const double SCALE = config.scale;

    vector<double> vlower, vupper,
                   glower, gupper,
                   variables(
                indexManager->getConstData().variableCount, 1e-4);
    initConstraintsLR(&glower, &gupper);
    initBoundsLR(&vlower, &vupper);

    if (canInitVariables) {
        assert(variables.size() == lastVariables.size());
        variables = lastVariables;
    } else {
        size_t variableCount =
            indexManager->getConstData().variableCount;
        variables.assign(variableCount, 0);
        for (size_t idx = 0; idx  < indexManager->getConstData().variableCount; ++idx) {
            if (vlower[idx] != -INF) { variables[idx] = vlower[idx]; }
            if (vupper[idx] != INF) { variables[idx] = vupper[idx]; }
            if (vlower[idx] != -INF && vupper[idx] != INF) {
                variables[idx] = vlower[idx] + (-vlower[idx] + vupper[idx]) / 10.;
            }
            if (vlower[idx] == -INF && vupper[idx] == INF) { variables[idx] = 0; };
        }
    }

    if (timeParameters.allotmentsSupplied) {
        // Restoring structures in case they were dumped.
        const auto& input =  config.inputManager->getConstData();
        const auto& indexMap = indexManager->getConstData();
        for (ui32 idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
            ui32 variableId = indexMap.allotmentToUIndex[idAllotment];
            auto* decision = decisionManagerToWrite->get();
            if (decision->allotmentAccepted[idAllotment]) {
                variables[variableId] = vlower[variableId] = vupper[variableId] = 1;
            } else {
                variables[variableId] = vlower[variableId] = vupper[variableId] = 0;
            }
        }
    }

    for (ui32 prevTime = 0; prevTime < timeParameters.timeEvent; ++prevTime) {
        // Restoring structures in case they were dumped.
        const auto& input = config.inputManager->getConstData();
        const auto& links = config.linksManager->getConstData();
        const auto& indexMap = indexManager->getConstData();
        const auto& event = input.events[prevTime];
        auto* decision = decisionManagerToWrite->get();
        if (event.type == EventType::cutoff) {
            const auto& arc = input.arcs[event.basedArc.value()];
            for (ui32 idItinerary : event.relatedItineraryIds) {
                // Processing Q_r
                ui32 takeQIndex = indexMap.idItineraryToQIndex[idItinerary];
                variables[takeQIndex] = vlower[takeQIndex] = vupper[takeQIndex] = decision->nonEmptyContainersQ[idItinerary] / SCALE;
                // Processing Z_r
                ui32 emptyZIndex = indexMap.idItineraryToZIndex[idItinerary];
                variables[emptyZIndex] = vlower[emptyZIndex] = vupper[emptyZIndex] = decision->emptyContainersZ[idItinerary] / SCALE;
                // Processing allotments
                for (ui32 idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                    ui32 takeIQIndex =
                        indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
                    ui32 placeIndex =
                        links.allotmentItineraryToPlace.at(idAllotment).at(idItinerary);

                    assert(decision->allotmentContainersQ[idAllotment][placeIndex].first == idItinerary);
                    double targetValue =
                        decision->allotmentContainersQ[idAllotment][placeIndex].second / SCALE;
                    if (!decision->allotmentAccepted[idAllotment]) {
                        targetValue = 0;
                    }
                    variables[takeIQIndex] = vlower[takeIQIndex] = vupper[takeIQIndex] = targetValue;
                }
            }
            // tracking variables y_a^H
            ui32 hireIndex = indexMap.arcToHired[arc.id];
            vlower[hireIndex] = vupper[hireIndex] = variables[hireIndex] = decision->hiredY[arc.id] / SCALE;
        } else if (event.type == EventType::pricing) {
            for (auto idItinerary : event.relatedItineraryIds) {
                auto* action = &currentActionManager->getConstData();
                ui32 bookingAmount = action->bookingsB[prevTime][idItinerary];
                ui32 valueIndex = indexMap.timeItineraryToDemandIndex[prevTime][idItinerary];
                variables[valueIndex] = vupper[valueIndex] = vlower[valueIndex] = double(bookingAmount) / SCALE;
            }
        } else if (event.type == EventType::arrival) {
            const auto& arc = input.arcs[event.basedArc.value()];
            const auto& node = input.nodes[arc.toNode];
            const auto& port = input.ports[node.portId];
            ui32 variableIndex = indexMap.timeToSIndex[event.relativeTime];
            variables[variableIndex] = vlower[variableIndex] = vupper[variableIndex] =
                double(decision->offHiredInPortS[prevTime][port.id]) / SCALE;
            for (const auto& otherPort : input.ports) {
                if (otherPort.id != port.id && decision->offHiredInPortS[prevTime][otherPort.id] != 0) {
                    throw std::logic_error("Deterministic Algorithm currently does not support offhiring in other places than arrival port!");
                }
            }
        }
    }

    // Creating problem and calling IPOPT.
    OptimizationConfig problemConfig;
    problemConfig.indexManager = indexManager;
    problemConfig.inputManager = config.inputManager;
    problemConfig.linksManager = config.linksManager;
    problemConfig.actionManager = currentActionManager;
    problemConfig.decisionManager = decisionManagerToWrite;
    problemConfig.scale = config.scale;
    problemConfig.useEnhancedVersion = config.useEnhancedVersion;
    problemConfig.updateTime = timeParameters.timeEvent;
    problemConfig.utilizationRatio = utilizationRatio;
    OptimizationProblem problem(problemConfig);

    logConstraints(vlower, vupper, glower, gupper, variables,
            indexManager->getConstData());

    std::string options = makeOptionsFromConfig(config);
    logOptions(options);

    CppAD::ipopt::solve_result<vector<double>> solution; // scaled
    CppAD::ipopt::solve<std::vector<double>, OptimizationProblem>(
          options, variables, vlower, vupper, glower, gupper, problem, solution
    );
    assert(variables.size() == solution.x.size());
    variables = solution.x;

    logger.debugStream() << "Expected ipopt-objective to get is: " <<  -solution.solve_result::obj_value;
    if (!timeParameters.allotmentsSupplied) {
        logger.debugStream() << "Allotments are not supplied. Ipopt will decide on its own.";

        // Restoring structures in case they were dumped.
        const auto& input = config.inputManager->getConstData();
        const auto& indexMap = indexManager->getConstData();

        const auto& solutionValues = variables;
        vector<bool> acceptedAllotments(input.allotments.size(), false);
        logger.debugStream() << "input.allotments.size() = " <<  input.allotments.size();

        for (ui32 idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
            ui32 variableId = indexMap.allotmentToUIndex[idAllotment];
            double value = solutionValues[variableId];
            logger.debugStream() << "To be floored: " <<  "allotment_id " << idAllotment << " value " << value;
            ui32 integer = floor(value + 0.01);
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
        for (ui32 nextTime = timeParameters.timeEvent;
                nextTime < input.events.size(); ++nextTime) {
            for (ui32 idPort = 0; idPort < input.ports.size(); ++idPort) {
                decision->offHiredInPortS[nextTime][idPort] = 0;
            }
            const auto& event = input.events[nextTime];
            if (event.type == EventType::cutoff) {

                const auto& arc = input.arcs[event.basedArc.value()];
                for (ui32 idItinerary : event.relatedItineraryIds) {
                    // Processing Q_r
                    ui32 takeQIndex = indexMap.idItineraryToQIndex[idItinerary];
                    double valueQ = floor(solution.x[takeQIndex] * SCALE);
                    decision->nonEmptyContainersQ[idItinerary] = ui32(valueQ);

                    // Processing Z_r (setting decision and bounds)
                    ui32 emptyZIndex = indexMap.idItineraryToZIndex[idItinerary];
                    double valueZ = floor(solution.x[emptyZIndex] * SCALE);
                    decision->emptyContainersZ[idItinerary] = ui32(valueZ);

                    // Processing allotments
                    for (ui32 idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                        ui32 takeIQIndex = indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
                        double valueIQ = floor(solution.x[takeIQIndex] * SCALE);
                        ui32 placeIndex = links.allotmentItineraryToPlace.at(idAllotment).at(idItinerary);
                        assert(decision->allotmentContainersQ[idAllotment][placeIndex].first == idItinerary);
                        if (!decision->allotmentAccepted[idAllotment]) {
                            valueIQ = 0;
                        }
                        decision->allotmentContainersQ[idAllotment][placeIndex].second = valueIQ;
                    }
                }

                // tracking variables y_a^H
                ui32 hireIndex = indexMap.arcToHired[arc.id];
                if (SCALE * solution.x[hireIndex] > 1e-2) {
                    decision->hiredY[arc.id] = ceil(SCALE * solution.x[hireIndex]);
                }

            } else if (event.type == EventType::pricing) {
                for (ui32 index = 0; index < event.relatedItineraryIds.size(); ++index) {
                    ui32 itineraryId = event.relatedItineraryIds[index];
                    ui32 variableId = indexMap.timeItineraryToDemandIndex[nextTime][itineraryId];
                    double demandValue = solutionValues[variableId];
                    double realValue = demandValue * SCALE;
                    const auto& demand = event.demands[index];
                    double price = INF;
                    if (demand.type == Demand::Type::exponential) {
                        price = log(demand.scale / realValue + 1e-50) / demand.sensitivity;
                    } else if (demand.type == Demand::Type::linear) {
                        price = (realValue - demand.additive) / demand.multiplicative;
                    } else {
                        throw std::logic_error("Unknown Demand Type");
                    }
                    auto& target = decision->prices[event.relativeTime][index];
                    assert(target.first == itineraryId);
                    target.second = price;
                }

            } else if (event.type == EventType::arrival) {
                const auto& arc = input.arcs[event.basedArc.value()];
                const auto& node = input.nodes[arc.toNode];
                const auto& port = input.ports[node.portId];
                ui32 variableIndex = indexMap.timeToSIndex[event.relativeTime];
                double offhired = floor(SCALE * solution.x[variableIndex]);
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
