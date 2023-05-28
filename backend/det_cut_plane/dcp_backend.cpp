// Implementation for DetCutPlaneBackend and related functions.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "dcp_backend.h"
#include "cbc_pre_map.h"
#include "../lagrangian_relaxation/index.h"

#include <string>
#include <random>
#include <limits>
#include <algorithm>
#include <iterator>
#include <memory>
#include <cassert>
#include <cmath>
#include <filesystem>

#include <coin-or/OsiSolverInterface.hpp>
#include <coin-or/OsiCbcSolverInterface.hpp>
#include <coin-or/OsiClpSolverInterface.hpp>

namespace sea {
namespace backend {

using std::unordered_map;
using std::vector;
using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;
using std::cout;
using std::endl;
using std::size_t;


// Functions for uniform building constraints, objective, revenue and derivatives.
void addMultiVarConstraint(const vector<CoefIndex>& variables,
                           double coefficient, unsigned constraintId,
                           vector<vector<CoefIndex>>& constraints) {
    for (auto variableIndex : variables) {
        if (constraints[variableIndex.index].empty()
                || constraints[variableIndex.index].back().index != constraintId) {
            constraints[variableIndex.index].push_back({0, constraintId});
        }
        // Multiple similar indices in one constraint
        constraints[variableIndex.index].back().coef += coefficient * variableIndex.coef;
    }
}

void addMultiVarConstraint(const vector<unsigned>& variables,
                           double coefficient, unsigned constraintId,
                           vector<vector<CoefIndex>>& constraints) {
    for (auto variableIndex : variables) {
        if (constraints[variableIndex].empty() ||
                constraints[variableIndex].back().index != constraintId) {
            constraints[variableIndex].push_back({0, constraintId});
        }
        // Multiple similar indices in one constraint.
        constraints[variableIndex].back().coef += coefficient;
    }
}

void addMultiVarObjective(const vector<CoefIndex>& variables,
        double coefficient, vector<double>& objective) {
    for (auto variableIndex : variables) {
        objective[variableIndex.index] += coefficient * variableIndex.coef;
    }
}

void addMultiVarObjective(
        const vector<unsigned>& variables, double coefficient, vector<double>& objective) {
    for (auto variableIndex : variables) {
        objective[variableIndex] += coefficient;
    }
}

double calculateRevenue(const Demand& demand, double demandValue) {
    double revenue = 0.0;
    if (demand.type == Demand::Type::linear) {
        revenue = demandValue * (demandValue - demand.additive) / demand.multiplicative;
    } else if (demand.type == Demand::Type::exponential) {
        assert(demand.scale > 0);
        const double LOG_TOLERANCE = 1e-200;
        revenue = demandValue * (-1.0 / demand.sensitivity * log(
                    LOG_TOLERANCE + demandValue / demand.scale));
    }
    return revenue;
}

double calculateRevenueDerivative(const Demand& demand, double demandValue) {
    if (demand.type == Demand::Type::linear) {
        return (2. * demandValue - demand.additive) / demand.multiplicative;
    } else {
        const double LOG_TOLERANCE = 1e-200;
        return (-1.0 / demand.sensitivity * log(
                    LOG_TOLERANCE + demandValue / demand.scale)) - 1.0 / demand.sensitivity;
    }
}


DetCutPlaneBackend::DetCutPlaneBackend(
        const DetCutPlaneBackendConfig& aConfig) : config (aConfig) {
    utilizationRatio = config.defaultUtilizationRatio;
    std::string filePath = "dcp_index_map_" + makeUniqueFileName();
    ManagerConfig indexMapConfig = {config.needMemory, filePath, true};
    indexManager = std::make_shared<DataManager<DcpIndexMap>>(indexMapConfig);
    dcpCreateIndexMap(config.inputManager->getConstData(), indexManager->getData());
}

void DetCutPlaneBackend::setupMainProblem() {
    auto& input = config.inputManager->getConstData();
    auto& links = config.linksManager->getConstData();
    auto& indexMap = indexManager->getConstData();
    int variableCount = indexMap.variableCount;
    int constraintCount = indexMap.constraintCount;

    cbcLastProblem.init(variableCount, constraintCount);
    initConstraintsLR(&cbcLastProblem.glower, &cbcLastProblem.gupper);
    initBoundsLR(&cbcLastProblem.vlower, &cbcLastProblem.vupper);

    vector<vector<unsigned>> bookings(input.itineraries.size());
    vector<vector<unsigned>> takens(input.itineraries.size());
    vector<vector<CoefIndex>> containers(input.ports.size());

    for (unsigned timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        const InputData::Event& event = input.events[timeNow];
        double timeDelta = 0.0;
        if (timeNow + 1 < input.events.size()) {
            const InputData::Event& nextEvent = input.events[timeNow + 1];
            timeDelta = nextEvent.realTime - event.realTime;
        }
        // Event-related recalculation.
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned relatedIndex = 0; relatedIndex < event.relatedItineraryIds.size();
                    ++relatedIndex) {
                unsigned idItinerary = event.relatedItineraryIds[relatedIndex];
                unsigned demandIndex = indexMap.timeItineraryToDemandIndex[timeNow][idItinerary];
                unsigned zi = indexMap.timeItineraryToZi[timeNow][idItinerary];
                bookings[idItinerary].push_back(demandIndex);
                cbcLastProblem.objective[zi] = 1; // objective += zi
            }
        } else {
            unsigned idBasedArc = event.basedArc.value();
            const auto& arc = input.arcs[idBasedArc];

            if (event.type == InputData::Event::Type::cutoff) {
                unsigned idNode = arc.fromNode;
                const auto& node = input.nodes[idNode];
                unsigned idPort = node.portId;
                const auto& port = input.ports[idPort];

                unsigned hireIndex = indexMap.arcToHired[idBasedArc];
                containers[idPort].push_back({+1, hireIndex});
                cbcLastProblem.objective[hireIndex] -= port.hiringCost;

                for (unsigned idItinerary : links.itinerariesFromArc[idBasedArc]) {

                    const auto& itinerary = input.itineraries[idItinerary];
                    unsigned qSpotIndex = indexMap.idItineraryToQIndex[idItinerary];
                    unsigned zIndex = indexMap.idItineraryToZIndex[idItinerary];

                    takens[idItinerary].push_back(qSpotIndex);
                    takens[idItinerary].push_back(zIndex);

                    containers[idPort].push_back({-1, qSpotIndex});
                    containers[idPort].push_back({-1, zIndex});

                    unsigned qIdConstraint = indexMap.spotQNConstraints[idItinerary];

                    addMultiVarConstraint(
                            bookings[idItinerary], itinerary.showRate.estimatedProba,
                            qIdConstraint, cbcLastProblem.coefByVar);
                    cbcLastProblem.coefByVar[qSpotIndex].push_back({-1, qIdConstraint});


                    addMultiVarObjective(bookings[idItinerary], -itinerary.returnPrice,
                        cbcLastProblem.objective);

                    addMultiVarObjective(bookings[idItinerary],
                        itinerary.showRate.estimatedProba * (
                            -itinerary.declineCost + itinerary.returnPrice),
                        cbcLastProblem.objective);

                    cbcLastProblem.objective[qSpotIndex] += (-itinerary.cost
                            + itinerary.declineCost - event.duration * port.storageCost);

                    cbcLastProblem.objective[zIndex] -= itinerary.emptyCost;

                    for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {

                        unsigned qAllotmentIndex =
                            indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];

                        unsigned uAllotmentIndex = indexMap.allotmentToUIndex[idAllotment];

                        takens[idItinerary].push_back(qAllotmentIndex);
                        containers[idPort].push_back({-1, qAllotmentIndex});

                        auto entryId = links.allotmentItineraryToEntry.at(
                                idAllotment).at(idItinerary);
                        const auto& entry = input.allotmentEntries[entryId];

                        double expectedAllotmentShow = entry.showRate.estimatedProba
                            * entry.productAmount;

                        cbcLastProblem.objective[uAllotmentIndex] += expectedAllotmentShow * (
                                -entry.cancellationPrice - itinerary.declineCost);
                        cbcLastProblem.objective[qAllotmentIndex] += (entry.price - itinerary.cost
                                + itinerary.declineCost - event.duration * port.storageCost);

                        double cancellationPrice = entry.productAmount * entry.cancellationPrice;
                        cbcLastProblem.objective[uAllotmentIndex] += cancellationPrice;

                        unsigned itineraryAllotmentQConstraintIndex =
                            indexMap.allotmentItineraryQConstraints[idItinerary][idAllotment];
                        cbcLastProblem.coefByVar[qAllotmentIndex].push_back(
                                {1, itineraryAllotmentQConstraintIndex});
                        cbcLastProblem.coefByVar[uAllotmentIndex].push_back(
                                {-expectedAllotmentShow, itineraryAllotmentQConstraintIndex});
                    }
                }

                // Containers constraint on cutoff
                unsigned cutoffPortConstraintIndex =
                    indexMap.portPositiveCutoffArcConstraints[idBasedArc];
                addMultiVarConstraint(containers[idPort], 1,
                        cutoffPortConstraintIndex, cbcLastProblem.coefByVar);
            } else if (event.type == InputData::Event::Type::arrival) {
                unsigned idNode = arc.toNode;
                const auto& node = input.nodes[idNode];
                const auto& port = input.ports[node.portId];
                auto offhiredIndex = indexMap.timeToSIndex[event.relativeTime];
                cbcLastProblem.objective[offhiredIndex] -= port.offHiringCost;

                containers[port.id].push_back({-1, offhiredIndex});
                for (unsigned idItinerary : links.itinerariesToArc[idBasedArc]) {
                    for (auto variableIndex : takens[idItinerary]) {
                        containers[port.id].push_back({1, variableIndex});
                    }
                }

                // Containers constraint on arrival
                unsigned arrivalPortConstraintIndex =
                    indexMap.portPositiveArrivalArcConstraints[idBasedArc];
                addMultiVarConstraint(containers[port.id], 1,
                        arrivalPortConstraintIndex, cbcLastProblem.coefByVar);
            }
        }
        // Common recalculation.
        for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
            const auto& port = input.ports[idPort];
            addMultiVarObjective(containers[idPort], -1 * timeDelta * port.storageCost,
                    cbcLastProblem.objective);
        }
    }

    // Capacity constraints
    for (unsigned idArc = 0; idArc < input.arcs.size(); ++idArc) {
        const auto& arc = input.arcs[idArc];
        if (arc.type == InputData::Arc::Type::travel) {
            vector<unsigned> insideArcIndices;
            for (unsigned idItinerary : links.itinerariesWithArc[idArc]) {
                std::copy(takens[idItinerary].begin(), takens[idItinerary].end(),
                        std::back_inserter(insideArcIndices));
            }
            unsigned constraintIndex = indexMap.arcCapacityConstraints[idArc];
            addMultiVarConstraint(
                    insideArcIndices, 1, constraintIndex, cbcLastProblem.coefByVar);
        }
    }

    // Constraints on final container count
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        unsigned constraintIndex = indexMap.finalContainerConstraints[idPort];
        addMultiVarConstraint(containers[idPort], 1, constraintIndex, cbcLastProblem.coefByVar);
        addMultiVarObjective(
                containers[idPort], -input.ports[idPort].offHiringCost, cbcLastProblem.objective);
    }

    // Group constraints
    for (unsigned idGroup = 0; idGroup < input.allotmentGroups.size(); ++idGroup) {
        const auto& group = input.allotmentGroups[idGroup];
        auto& constraintIndex = indexMap.groupConstraints[idGroup];
        vector<unsigned> uAllotmentIndices(group.size());
        std::transform(group.begin(), group.end(), uAllotmentIndices.begin(),
                [&](unsigned idx) { return indexMap.allotmentToUIndex[idx]; });
        addMultiVarConstraint(uAllotmentIndices, 1, constraintIndex, cbcLastProblem.coefByVar);
    }
}

double DetCutPlaneBackend::runCbc() {
    OsiCbcSolverInterface solver;
    cbcLastProblem.setupCbcSolver(solver);

    const auto& input = config.inputManager->getConstData();
    auto& indexMap = indexManager->getData();

    for (const auto& allotment : input.allotments) {
        solver.setInteger(indexMap.allotmentToUIndex[allotment.id]);
    }
    CbcModel model(solver);
    model.setIntegerTolerance(config.integerTolerance);
    unsigned cbcLogLevel = config.cbcLogLevel;
    model.setLogLevel(cbcLogLevel);

    if (lastSolution.size() == indexMap.variableCount) {
        // Check that no bounds are violated.
        for (std::size_t idx = 0; idx < indexMap.variableCount; idx++) {
            const double CHECK_TOLERANCE = 1e-2;
            assert(lastSolution[idx] <= cbcLastProblem.vupper[idx] + CHECK_TOLERANCE);
            assert(lastSolution[idx] + CHECK_TOLERANCE >= cbcLastProblem.vlower[idx]);
        }

        // Check only if objective looks infeasible.
        const double THRESHOLD_SET_OBJ = 1e40;
        bool checkFlag = (preparedSolutionObjective > THRESHOLD_SET_OBJ);

        // Set previous solution.
        // Designed for hot start.
        model.setBestSolution(lastSolution.data(),
                lastSolution.size(),
                preparedSolutionObjective, checkFlag);
    }
    if (lastSolution.size() != size_t(indexMap.variableCount)) {
        lastSolution.assign(indexMap.variableCount, 0);
        preparedSolutionObjective = 0;
    }
    const double N_SECONDS_THRESHOLD = 30. * 60;
    model.setMaximumSeconds(N_SECONDS_THRESHOLD);
    // Solve the problem.
    {
        // This code is about redirecting stdout to a file, since
        // no direct handle is provided inside Cbc.

        int fileDescriptor; fpos_t position;
        fflush(stdout);
        fgetpos(stdout, &position);
        fileDescriptor = dup(fileno(stdout));

        auto folderPath = std::filesystem::path("dcp");
        if (!std::filesystem::exists(folderPath)) {
            std::filesystem::create_directories(folderPath);
        }

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-result"
        freopen("dcp/cbc.log", "a+", stdout);
        #pragma GCC diagnostic pop
        printf("Deterministic cutting plane branch & bound."
                "Started computation.\n");

        model.branchAndBound();

        // This code closes the file, and restores stdout.
        printf("Deterministic cutting plane branch & bound."
                "Finished computation.\n");
        fflush(stdout);
        dup2(fileDescriptor, fileno(stdout));
        close(fileDescriptor);
        clearerr(stdout);
        fsetpos(stdout, &position);
    }
    auto solution = model.getColSolution();
    double objectiveEstimation = model.getObjValue();
    if (model.isProvenInfeasible() || model.isProvenDualInfeasible()) {
        objectiveEstimation = std::fabs(objectiveEstimation);
    }
    preparedSolutionObjective = objectiveEstimation;
    if (config.stopOnInfeasible) {
        if (model.isProvenInfeasible() || model.isProvenDualInfeasible()) {
            return -1;
        }
    }
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        double finalCount = input.ports[idPort].finalContainerCount;
        objectiveEstimation += finalCount * input.ports[idPort].offHiringCost;
    }
    // Save solution.
    CoinDisjointCopyN(solution, indexMap.variableCount, lastSolution.data());

    for (std::size_t idx = 0; idx < indexMap.variableCount; ++idx) {
        const double CHECK_TOLERANCE = 1e-2;
        if (lastSolution[idx] > cbcLastProblem.vupper[idx]) {
            assert(fabs(lastSolution[idx] - cbcLastProblem.vupper[idx]) < CHECK_TOLERANCE);
        }
        if (lastSolution[idx] < cbcLastProblem.vlower[idx]) {
            assert(fabs(lastSolution[idx] - cbcLastProblem.vlower[idx]) < CHECK_TOLERANCE);
        }
        lastSolution[idx] = std::min(cbcLastProblem.vupper[idx], lastSolution[idx]);
        lastSolution[idx] = std::max(cbcLastProblem.vlower[idx], lastSolution[idx]);
    }
    for (const auto& allotment : input.allotments) {
        unsigned allotmentVariableId = indexMap.allotmentToUIndex[allotment.id];
        lastSolution[allotmentVariableId] = round(lastSolution[allotmentVariableId]);
    }
    return objectiveEstimation;
}


double DetCutPlaneBackend::checkConstraintsAndBounds() {
    double maxViolation = 0.;

    auto& indexMap = indexManager->getConstData();
    vector<double> constrValues(indexMap.constraintCount);
    for (unsigned idx = 0; idx < cbcLastProblem.coefByVar.size(); idx++) {
        for (const CoefIndex& coeffIndex : cbcLastProblem.coefByVar[idx]) {
            assert(!std::isnan(coeffIndex.coef));
            assert(!std::isnan(lastSolution[idx]));
            constrValues[coeffIndex.index] += lastSolution[idx] * coeffIndex.coef;
        }
    }
    for (unsigned idx = 0; idx < constrValues.size(); idx++) {
        double value = constrValues[idx];
        double& lower = cbcLastProblem.glower[idx];
        double& upper = cbcLastProblem.gupper[idx];
        assert(!std::isnan(lower));
        assert(!std::isnan(upper));
        assert(!std::isnan(value));
        if (value > upper) {
            maxViolation = std::max(maxViolation, value - upper);
            if (config.tolerateConstraints) {
                upper = value;
            }
        }
        if (value < lower) {
            maxViolation = std::max(maxViolation, lower - value);
            if (config.tolerateConstraints) {
                lower = value;
            }
        }
    }
    for (unsigned ind = 0; ind < indexMap.variableCount; ++ind) {
        double lower = cbcLastProblem.vlower[ind];
        double upper = cbcLastProblem.vupper[ind];
        double value = lastSolution[ind];
        assert(!std::isnan(lower));
        assert(!std::isnan(upper));
        assert(!std::isnan(value));
        if (value < lower) {
            maxViolation = std::max(maxViolation, lower - value);
        }
        if (value > upper) {
            maxViolation = std::max(maxViolation, value - upper);
        }
    }
    return maxViolation;
}


double DetCutPlaneBackend::calcError() {
    const auto& input = config.inputManager->getConstData();
    const auto& indexMap = indexManager->getConstData();

    double result = 0;
    for (unsigned timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        const auto& event = input.events[timeNow];
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned relatedIndex = 0;
                    relatedIndex < event.relatedItineraryIds.size();
                    ++relatedIndex) {
                unsigned idItinerary = event.relatedItineraryIds[relatedIndex];
                unsigned demandIndex =
                    indexMap.timeItineraryToDemandIndex[timeNow][idItinerary];
                unsigned zi = indexMap.timeItineraryToZi[timeNow][idItinerary];
                const auto& demand = event.demands[relatedIndex];
                result += fabs(lastSolution[zi] - calculateRevenue(
                        demand, lastSolution[demandIndex]));
            }
        }
    }
    return result;
}

void DetCutPlaneBackend::addConstraints() {
    const auto& input = config.inputManager->getConstData();
    auto& indexMap = indexManager->getData();
    indexMap.constraintCount += indexMap.demandZCount;
    cbcLastProblem.reserveNewConstraints(indexMap.demandZCount);

    for (unsigned timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        const auto& event = input.events[timeNow];
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned relatedIndex = 0; relatedIndex < event.relatedItineraryIds.size();
                    ++relatedIndex) {
                unsigned idItinerary = event.relatedItineraryIds[relatedIndex];
                unsigned demandIndex = indexMap.timeItineraryToDemandIndex[timeNow][idItinerary];

                unsigned zi = indexMap.timeItineraryToZi[timeNow][idItinerary];
                const auto& demand = event.demands[relatedIndex];

                auto demandVal = lastSolution[demandIndex];

                cbcLastProblem.coefByVar[zi].push_back(
                        {1, indexMap.constraintCount - indexMap.demandZCount + zi});
                cbcLastProblem.coefByVar[demandIndex].push_back(
                        {-calculateRevenueDerivative(demand, demandVal),
                    indexMap.constraintCount - indexMap.demandZCount + zi});

                cbcLastProblem.gupper[indexMap.constraintCount - indexMap.demandZCount + zi] =
                    calculateRevenue(demand, demandVal)
                    - calculateRevenueDerivative(demand, demandVal) * demandVal;

                double oldValue = lastSolution[zi];
                lastSolution[zi] = std::min(lastSolution[zi], calculateRevenue(demand, demandVal));
                preparedSolutionObjective += (lastSolution[zi] - oldValue);
            }
        }
    }
}

void DetCutPlaneBackend::genRandomSolution() {
    const auto& input = config.inputManager->getConstData();
    const auto& indexMap = indexManager->getConstData();
    std::default_random_engine generator(config.seed);

    if (lastSolution.size() != size_t(indexMap.variableCount)) {
        lastSolution.assign(indexMap.variableCount, 0);
    }
    for (unsigned timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        const auto& event = input.events[timeNow];
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned relatedIndex = 0;
                    relatedIndex < event.relatedItineraryIds.size(); ++relatedIndex) {
                unsigned idItinerary = event.relatedItineraryIds[relatedIndex];
                unsigned demandIndex = indexMap.timeItineraryToDemandIndex[timeNow][idItinerary];
                const auto& demand = event.demands[relatedIndex];

                double maxDemand = 0.;
                if (demand.type == Demand::Type::linear) {
                    maxDemand = demand.additive;
                } else if (demand.type == Demand::Type::exponential) {
                    maxDemand = demand.scale;
                } else {
                    throw std::logic_error("Unknown demand type!");
                }
                std::uniform_real_distribution<double> distribution(0, maxDemand);
                lastSolution[demandIndex] = distribution(generator);
            }
        }
    }
}

void DetCutPlaneBackend::fillDecision(Decision* decision) {
    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();
    const auto& indexMap = indexManager->getConstData();
    // Writing results to decision.
    const auto& solutionValues = lastSolution;
    for (unsigned nextTime = 0; nextTime < input.events.size(); ++nextTime) {
        for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
            decision->offHiredInPortS[nextTime][idPort] = 0;
        }
        const auto& event = input.events[nextTime];
        if (event.type == EventType::cutoff) {
            const auto& arc = input.arcs[event.basedArc.value()];
            for (unsigned idItinerary : event.relatedItineraryIds) {
                // Processing Q_r
                const double EPS = 1e-3;
                unsigned takeQIndex = indexMap.idItineraryToQIndex[idItinerary];
                double valueQ = floor(lastSolution[takeQIndex] + EPS);
                decision->nonEmptyContainersQ[idItinerary] = unsigned(valueQ);

                // Processing Z_r (setting decision and bounds)
                unsigned emptyZIndex = indexMap.idItineraryToZIndex[idItinerary];
                double valueZ = floor(lastSolution[emptyZIndex] + EPS);
                decision->emptyContainersZ[idItinerary] = unsigned(valueZ);

                // Processing allotments
                for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                    unsigned takeIQIndex = indexMap.allotmentItineraryToQIndex[
                        idAllotment][idItinerary];
                    double valueIQ = floor(lastSolution[takeIQIndex] + EPS);
                    unsigned placeIndex = links.allotmentItineraryToPlace.at(
                            idAllotment).at(idItinerary);
                    assert(decision->allotmentContainersQ[idAllotment][placeIndex].first
                            == idItinerary);
                    if (!decision->allotmentAccepted[idAllotment]) {
                        valueIQ = 0;
                    }
                    decision->allotmentContainersQ[idAllotment][placeIndex].second = valueIQ;
                }
            }
            // tracking variables y_a^H
            unsigned hireIndex = indexMap.arcToHired[arc.id];
            const double EPS_THRESHOLD = 1e-2;
            if (lastSolution[hireIndex] > EPS_THRESHOLD) {
                decision->hiredY[arc.id] = ceil(lastSolution[hireIndex]);
            }
        } else if (event.type == EventType::pricing) {
            for (unsigned index = 0; index < event.relatedItineraryIds.size(); ++index) {
                unsigned itineraryId = event.relatedItineraryIds[index];
                unsigned variableId = indexMap.timeItineraryToDemandIndex[nextTime][itineraryId];
                double demandValue = solutionValues[variableId];
                double realValue = demandValue;
                const auto& demand = event.demands[index];
                const double INF = std::numeric_limits<double>::max();
                double price = INF;
                if (demand.type == Demand::Type::exponential) {
                    const double LOG_EPS = 1e-50;
                    price = log(demand.scale / realValue + LOG_EPS) / demand.sensitivity;
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
            const double EPS = 1e-3;
            unsigned variableIndex = indexMap.timeToSIndex[event.relativeTime];
            double offhired = floor(lastSolution[variableIndex] + EPS);
            decision->offHiredInPortS[nextTime][port.id] = offhired;
        } else {
            throw std::logic_error("Event type is not supported");
        }
    }
}

DecisionManagerPtr DetCutPlaneBackend::provideAllotments(double* objectiveValue) {
    std::string filePath = "decision_" + makeUniqueFileName();
    ManagerConfig decisionConfig = {config.needMemory, filePath, true};
    vector<bool> answer;

    DecisionManagerPtr decisionManager = std::make_shared<DataManager<Decision>>(decisionConfig);
    createDecision(config.inputManager->getConstData(), decisionManager->get());

    setupMainProblem();
    cbcLastProblem.logCbcConstraints(indexManager->getConstData());

    for (std::size_t idx = 0; idx < config.initialPlanes; ++idx) {
        genRandomSolution();
        addConstraints();
    }
    auto& indexMap = indexManager->getData();
    unsigned initConstraintCount = indexMap.constraintCount;
    unsigned iterCount = 0;
    double error = 0, constraintError = 0, accumulatedConstraintError = 0;
    lastSolution.clear();
    double objectiveEstimation = 0;

    do {
        if (lastSolution.size()) {
            addConstraints();
        }
        objectiveEstimation = runCbc();

        iterCount++;
        constraintError = checkConstraintsAndBounds();
        accumulatedConstraintError += constraintError;
        error = calcError();
        logIterationInProvideAllotments(iterCount, error, constraintError,
                accumulatedConstraintError, objectiveEstimation);

        if (objectiveEstimation < 0) {
             break;
        }
    } while (iterCount < config.iterations && error > config.needError);

    const auto& input = config.inputManager->getConstData();

    vector<bool> response(input.allotments.size(), false);
    for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        unsigned uIndex = indexMap.allotmentToUIndex[idAllotment];
        response[idAllotment] = lastSolution[uIndex];
    }
    auto* decision = decisionManager->get();
    decision->allotmentAccepted = response;
    fillDecision(decision);

    indexMap.constraintCount = initConstraintCount;
    if (objectiveValue != nullptr) {
        *objectiveValue = objectiveEstimation;
    }
    return decisionManager;
}

void DetCutPlaneBackend::initBoundsLR(vector<double>* vlowerPtr, vector<double>* vupperPtr) {
    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();
    const auto& indexMap = indexManager->getConstData();
    assert(vupperPtr != nullptr && vlowerPtr != nullptr);

    auto& vupper = *vupperPtr;
    auto& vlower = *vlowerPtr;

    const double INF = std::numeric_limits<double>::max();
    vupper.assign(indexMap.variableCount, INF);
    vupper.shrink_to_fit();
    vlower.assign(indexMap.variableCount, -INF);
    vlower.shrink_to_fit();

    // Q_r, Z_r, Q_ir
    for (unsigned idItinerary = 0; idItinerary < input.itineraries.size(); ++idItinerary) {
        unsigned indexQr = indexMap.idItineraryToQIndex[idItinerary];
        updateLower<double>(vlower[indexQr], 0.);
        unsigned indexZr = indexMap.idItineraryToZIndex[idItinerary];
        updateLower<double>(vlower[indexZr], 0.);
        for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
            unsigned indexQir = indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
            updateLower<double>(vlower[indexQir], 0.);
            unsigned indexEntry = links.allotmentItineraryToEntry.at(idAllotment).at(idItinerary);
            const auto& entry = input.allotmentEntries[indexEntry];
            double expectedShow = entry.productAmount * entry.showRate.estimatedProba;
            vupper[indexQir] = expectedShow;
        }
    }
    // u_i
    for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        unsigned indexUi = indexMap.allotmentToUIndex[idAllotment];
        updateLower<double>(vlower[indexUi], 0.0);
        updateUpper<double>(vupper[indexUi], 1.0);
    }
    // d_t, s_t, y_a^H
    for (const auto& event : input.events) {
        unsigned relativeTime = event.relativeTime;
        // d_t
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned index = 0; index < event.relatedItineraryIds.size(); ++index) {
                unsigned idItinerary = event.relatedItineraryIds[index];
                const auto& demand = event.demands[index];
                unsigned variableIndex = indexMap.timeItineraryToDemandIndex[
                    relativeTime][idItinerary];
                double& lower = vlower[variableIndex];
                double& upper = vupper[variableIndex];

                updateLower<double>(lower, 0.);
                if (demand.type == Demand::Type::linear) {
                    updateUpper(upper, demand.additive);
                } else if (demand.type == Demand::Type::exponential) {
                    updateUpper<double>(upper, demand.scale);
                    const double EXP_BOUND = 1e-100;
                    updateLower<double>(lower, EXP_BOUND);
                } else {
                    throw std::logic_error("Unsupported demand type");
                }
            }
        // s_t
        } else if (event.type == InputData::Event::Type::arrival) {
            unsigned variableIndex = indexMap.timeToSIndex[relativeTime];
            updateLower<double>(vlower[variableIndex], 0.);
        // y_a^H
        }  else if (event.type == InputData::Event::Type::cutoff) {
            unsigned basedArcId = event.basedArc.value();
            unsigned variableIndex = indexMap.arcToHired[basedArcId];
            updateLower<double>(vlower[variableIndex], 0.);
        } else {
            throw std::logic_error("Unsupported event type");
        }
    }
    // z_i bounds
    for (unsigned variableIndex = indexMap.variableCount - indexMap.demandZCount;
            variableIndex < indexMap.variableCount; variableIndex++) {
        updateLower<double>(vlower[variableIndex], 0.);
    }
    for (unsigned timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        const auto& event = input.events[timeNow];
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned relatedIndex = 0; relatedIndex < event.relatedItineraryIds.size();
                    ++relatedIndex) {
                unsigned idItinerary = event.relatedItineraryIds[relatedIndex];
                unsigned zi = indexMap.timeItineraryToZi[timeNow][idItinerary];
                const auto& demand = event.demands[relatedIndex];

                if (demand.type == Demand::Type::linear) {
                    updateUpper(vupper[zi], calculateRevenue(demand, demand.additive / 2));
                } else if (demand.type == Demand::Type::exponential) {
                    double optimalDemand = demand.scale * std::exp(-1.0);
                    updateUpper(vupper[zi], calculateRevenue(demand, optimalDemand));
                }

            }
        }
    }
}

void DetCutPlaneBackend::initConstraintsLR(vector<double>* glowerPtr, vector<double>* gupperPtr) {
    const auto& indexMap = indexManager->getConstData();
    const auto& input = config.inputManager->getConstData();

    assert(glowerPtr != nullptr && gupperPtr != nullptr);

    auto& glower = *glowerPtr;
    auto& gupper = *gupperPtr;

    const double INF = std::numeric_limits<double>::max();
    glower.assign(indexMap.constraintCount, -INF);
    glower.shrink_to_fit();
    gupper.assign(indexMap.constraintCount, INF);
    gupper.shrink_to_fit();

    // arc capacity constraints
    for (const auto& arc : input.arcs) {
        if (arc.type == InputData::Arc::Type::travel) {
            unsigned constraintIndex = indexMap.arcCapacityConstraints[arc.id];
            updateUpper<double>(gupper[constraintIndex],
                    input.vessels[arc.vesselId.value()].capacity * utilizationRatio);
        }
    }
    // positive in ports for cutoff and arrival
    for (const auto& event : input.events) {
        if (event.type == InputData::Event::Type::cutoff) {
            unsigned idBasedArc = event.basedArc.value();
            unsigned constraintIndex = indexMap.portPositiveCutoffArcConstraints[idBasedArc];
            updateLower<double>(glower[constraintIndex], 0.);
        } else if (event.type == InputData::Event::Type::arrival) {
            unsigned idBasedArc = event.basedArc.value();
            unsigned constraintIndex = indexMap.portPositiveArrivalArcConstraints[idBasedArc];
            updateLower<double>(glower[constraintIndex], 0.);
        }
    }
    // spotQN constraints
    for (const auto& itinerary : input.itineraries) {
        unsigned idItinerary = itinerary.id;
        unsigned constraintIndex = indexMap.spotQNConstraints[idItinerary];
        updateLower<double>(glower[constraintIndex], 0.);
    }
    // final container constraints
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        unsigned constraintIndex = indexMap.finalContainerConstraints[idPort];
        double finalCount = input.ports[idPort].finalContainerCount;
        updateUpper<double>(gupper[constraintIndex], finalCount);
    }
    // group constraints
    for (unsigned idGroup = 0; idGroup < input.allotmentGroups.size(); ++idGroup) {
        unsigned constraintIndex = indexMap.groupConstraints[idGroup];
        updateLower<double>(glower[constraintIndex], 0.);
        updateUpper<double>(gupper[constraintIndex], 1.);
    }
    // allotmentQ constraints
    for (const auto& allotmentQConstraints : indexMap.allotmentItineraryQConstraints) {
        for (const auto& qConstraintIndex : allotmentQConstraints) {
            updateUpper<double>(gupper[qConstraintIndex], 0.);
        }
    }
}


} // namespace backend
} // namespace sea

