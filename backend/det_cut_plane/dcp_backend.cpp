#include "dcp_backend.h"
#include "cbc_pre_map.h"
#include "../lagrangian_relaxation/index.h"
#include "../../logging/logging.h"

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

const double INF = std::numeric_limits<double>::max();
const double THRESHOLD_SET_OBJ = 1e40;
const double LOG_TOLERANCE = 1e-200;
const double EXP_BOUND = 1e-100;
const double CHECK_TOLERANCE = 1e-2;
const double EPS = 1e-3;

// Useful functions.
void addMultiVarConstraint(const vector<CoefIndex>& vars,
                           double coef,
                           unsigned constraintId,
                           vector<vector<CoefIndex>>& constraints) {
    for (auto varIndex : vars) {
        if (constraints[varIndex.index].empty()
                || constraints[varIndex.index].back().index != constraintId) {
            constraints[varIndex.index].push_back({0, constraintId});
        }
        // Multiple similar indices in one constraint
        constraints[varIndex.index].back().coef += coef * varIndex.coef;
    }
}

void addMultiVarConstraint(const vector<unsigned>& vars,
                           double coef, unsigned constraintId,
                           vector<vector<CoefIndex>>& constraints) {
    for (auto varIndex : vars) {
        if (constraints[varIndex].empty() || constraints[varIndex].back().index != constraintId) {
            constraints[varIndex].push_back({0, constraintId});
        }
        constraints[varIndex].back().coef += coef; // Multiple similar indices in one constraint
    }
}

void addMultiVarObjective(const vector<CoefIndex>& vars, double coef, vector<double>& objective) {
    for (auto varIndex : vars) {
        objective[varIndex.index] += coef * varIndex.coef;
    }
}

void addMultiVarObjective(const vector<unsigned>& vars, double coef, vector<double>& objective) {
    for (auto varIndex : vars) {
        objective[varIndex] += coef;
    }
}

double calcRevenue(Demand demand, double demandVal) {
    double revenue = 0.0;
    if (demand.type == Demand::Type::linear) {
        revenue = demandVal * (demandVal - demand.additive) / demand.multiplicative;
    } else if (demand.type == Demand::Type::exponential) {
        assert(demand.scale > 0);
        revenue = demandVal * (- 1.0 / demand.sensitivity * log(
                    LOG_TOLERANCE + demandVal / demand.scale));
    }

    return revenue;
}

double calcRevenueDeriv(Demand demand, double demandVal) {
    if (demand.type == Demand::Type::linear) {
        return (2 * demandVal - demand.additive) / demand.multiplicative;
    } else {
        return (- 1.0 / demand.sensitivity * log(
                    LOG_TOLERANCE + demandVal / demand.scale)) - 1.0 / demand.sensitivity;
    }
}


DetCutPlaneBackend::DetCutPlaneBackend(
        const DetCutPlaneBackendConfig& aConfig)
        : config (aConfig) {
    utilizationRatio = config.defaultUtilizationRatio;
    std::string filePath = "dcp_index_map_" + makeUniqueFileName();
    ManagerConfig indexMapConfig =
        {config.needMemory, filePath, true};
    indexManager = std::make_shared<DataManager<DcpIndexMap>>(indexMapConfig);
    dcpCreateIndexMap(config.inputManager->getConstData(),
            indexManager->getData());

    auto logger = logging::getBackendSubLogger(BackendType::DET_CUT_PLANE)
        << log4cpp::Priority::DEBUG;
    printIndexMapStats(logger, indexManager->getConstData());
}

void DetCutPlaneBackend::setupMainProblem() {

    auto& logger = logging::getBackendSubLogger(BackendType::DET_CUT_PLANE);
    logger.debug("DetCutPlaneBackend::recalcuate is called.");

    auto& input = config.inputManager->getConstData();
    auto& links = config.linksManager->getConstData();
    auto& indexMap = indexManager->getConstData();
    int varCount = indexMap.variableCount;
    int constraintCount = indexMap.constraintCount;

    cbcLastProblem.init(varCount, constraintCount);
    initConstraintsLR(&cbcLastProblem.glower, &cbcLastProblem.gupper);
    initBoundsLR(&cbcLastProblem.vlower, &cbcLastProblem.vupper);

    vector<vector<unsigned>> bookings(input.itineraries.size());
    vector<vector<unsigned>> takens(input.itineraries.size());
    vector<vector<CoefIndex>> containers(input.ports.size());

    auto debugStream = logger.getStream(log4cpp::Priority::DEBUG);
    for (unsigned timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        debugStream << "Considering time = " << timeNow << "\n";
        const InputData::Event& event = input.events[timeNow];
        double timeDelta = 0.0;
        if (timeNow + 1 < input.events.size()) {
            const InputData::Event& nextEvent = input.events[timeNow + 1];
            timeDelta = nextEvent.realTime - event.realTime;
        }
        debugStream << "Computed timeDelta as " << timeDelta << "\n";

        // Event-related recalculation.
        if (event.type == InputData::Event::Type::pricing) {
            debugStream << "Considering pricing event." << "\n";
            debugStream << "Will now iterate over event.relatedItineraryIds.\n";
            for (unsigned relatedIndex = 0; relatedIndex < event.relatedItineraryIds.size();
                    ++relatedIndex) {

                unsigned idItinerary = event.relatedItineraryIds[relatedIndex];
                debugStream << "Related idItinerary is " << idItinerary << "\n";

                unsigned demandIndex = indexMap.timeItineraryToDemandIndex[timeNow][idItinerary];
                debugStream << "Calculated demandIndex from indexMap as " << demandIndex << "\n";

                unsigned zi = indexMap.timeItineraryToZi[timeNow][idItinerary];

                debugStream << "Adding demandVar to bookings.\n";
                // bookings[idItinerary] += demandVar;
                bookings[idItinerary].push_back(demandIndex);
                cbcLastProblem.objective[zi] = 1; // objective += zi
            }
        } else {
            debugStream << "Considering non-pricing event.\n";

            unsigned idBasedArc = event.basedArc.value();
            const auto& arc = input.arcs[idBasedArc];

            debugStream << "Computed based arc.\n";

            if (event.type == InputData::Event::Type::cutoff) {

                debugStream << "Considering cut-off event.\n";

                unsigned idNode = arc.fromNode;
                debugStream << "Arc.fromNode = " << idNode << "\n";

                const auto& node = input.nodes[idNode];

                unsigned idPort = node.portId;
                debugStream << "Node.portid = " << idPort << "\n";

                const auto& port = input.ports[idPort];

                unsigned hireIndex = indexMap.arcToHired[idBasedArc];

                // containers[idPort] += hired; // TODO deal with containers
                containers[idPort].push_back({+1, hireIndex});
                debugStream << "Adding to containers[idPort] amount of hired containers.\n";

                // objective -= port.hiringCost * hired;
                cbcLastProblem.objective[hireIndex] -= port.hiringCost;

                debugStream << "For loop over links.itinerariesFromArc[idBasedArc] \n";
                for (unsigned idItinerary : links.itinerariesFromArc[idBasedArc]) {
                    debugStream << "Considering itinerary with id = " << idItinerary << "\n";

                    const auto& itinerary = input.itineraries[idItinerary];
                    unsigned qSpotIndex = indexMap.idItineraryToQIndex[idItinerary];
                    debugStream << "Computing "
                        << "unsigned qSpotIndex = indexMap.idItineraryToQIndex[idItinerary]" << "\n";
                    debugStream << "qSpotIndex = " << qSpotIndex << "\n";

                    unsigned zIndex = indexMap.idItineraryToZIndex[idItinerary];
                    debugStream << "zIndex = " << zIndex << "\n";

                    // const AD<double>& qSpot = variables[qSpotIndex];
                    // const AD<double>& zEmpty = variables[zIndex];

                    takens[idItinerary].push_back(qSpotIndex); // takens[idItinerary] += qSpot;
                    takens[idItinerary].push_back(zIndex); // takens[idItinerary] += zEmpty;

                    debugStream << "Adding qSpot and zEmpty to takens[idItinerary]\n";

                    containers[idPort].push_back({-1, qSpotIndex}); // containers[idPort] -= qSpot;
                    containers[idPort].push_back({-1, zIndex}); // containers[idPort] -= zEmpty;
                    debugStream << "Subtracting qSpot and zEmpty from containers[idPort]\n";

                    unsigned qIdConstraint = indexMap.spotQNConstraints[idItinerary];
                    debugStream << "Taking qIdConstraint" <<
                        " as indexMap.spotQNConstraints[idItinerary]" << "\n";


                    // expectedShow = bookings[idItinerary] * itinerary.showRate.estimatedProba;

                    // spotQNConstraint = expectedShow - qSpot; // inf >= spotQNConstraint >= 0
                    addMultiVarConstraint(bookings[idItinerary], itinerary.showRate.estimatedProba,
                        qIdConstraint, cbcLastProblem.coefByVar);
                    cbcLastProblem.coefByVar[qSpotIndex].push_back({-1, qIdConstraint});

                    debugStream << "Assiging spotQNConstraint as show >= qSpot" << "\n";

                    // objective -= bookings[idItinerary] * itinerary.returnPrice;
                    addMultiVarObjective(bookings[idItinerary], -itinerary.returnPrice,
                        cbcLastProblem.objective);
                    debugStream << "subtracting itinerary.returnPrice from objective" << "\n";

                    // objective += expectedShow * (-itinerary.declineCost + itinerary.returnPrice);
                    addMultiVarObjective(bookings[idItinerary],
                        itinerary.showRate.estimatedProba * (-itinerary.declineCost + itinerary.returnPrice),
                        cbcLastProblem.objective);
                    debugStream << " adding diff return - decline" << "\n";

                    // objective += qSpot * (-itinerary.cost + itinerary.declineCost - event.duration * port.storageCost);
                    cbcLastProblem.objective[qSpotIndex] += (-itinerary.cost + itinerary.declineCost - event.duration * port.storageCost);
                    debugStream << "adding qSpot" << "\n";

                    // objective -=  zEmpty * itinerary.emptyCost;
                    cbcLastProblem.objective[zIndex] -= itinerary.emptyCost;
                    debugStream << "removing zEmpty * itinerary.emptyCost" << "\n";

                    debugStream << "running cycle by allotmentsWithItinerary for itinerary " << idItinerary <<  "\n";
                    for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                        debugStream << "idAllotment = " << idAllotment << "\n";

                        unsigned qAllotmentIndex = indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
                        debugStream << "qAllotmentIndex = " << qAllotmentIndex << "\n";

                        unsigned uAllotmentIndex = indexMap.allotmentToUIndex[idAllotment];
                        debugStream << "uAllotmentIndex = " << uAllotmentIndex << "\n";

                        takens[idItinerary].push_back(qAllotmentIndex); // takens[idItinerary] += qAllotment
                        containers[idPort].push_back({-1, qAllotmentIndex}); // containers[idPort] -= qAllotment;

                        auto entryId = links.allotmentItineraryToEntry.at(idAllotment).at(idItinerary);
                        debugStream << "allotmentId = " << idAllotment << " idItinerary = "
                            << idItinerary << " idAllotmentEntry = " << entryId << "\n";
                        const auto& entry = input.allotmentEntries[entryId];

                        debugStream << "computing expectedAllotmentShow: " << "\n";
                        double expectedAllotmentShow = entry.showRate.estimatedProba * entry.productAmount;
                        debugStream << "scaled expectedAllotmentShow = " << expectedAllotmentShow << "\n";

                        // objective += uAllotment * expectedAllotmentShow * (-entry.cancellationPrice - itinerary.declineCost);
                        cbcLastProblem.objective[uAllotmentIndex] += expectedAllotmentShow * (-entry.cancellationPrice - itinerary.declineCost);
                        debugStream << "modified objective to expectedAllotmentShow";

                        // objective += qAllotment * (entry.price - itinerary.cost + itinerary.declineCost - event.duration * port.storageCost);
                        cbcLastProblem.objective[qAllotmentIndex] += (entry.price - itinerary.cost + itinerary.declineCost - event.duration * port.storageCost);
                        debugStream << "modified objective wrt qAllotment" << "\n";

                        // double cancellationPrice = entry.productAmount * entry.cancellationPrice;
                        double cancellationPrice = entry.productAmount * entry.cancellationPrice;
                        // objective += uAllotment * cancellationPrice;
                        cbcLastProblem.objective[uAllotmentIndex] += cancellationPrice;

                        unsigned itineraryAllotmentQConstraintIndex = indexMap.allotmentItineraryQConstraints[idItinerary][idAllotment];
                        debugStream << "itineraryAllotmentQConstraintIndex = " << itineraryAllotmentQConstraintIndex << "\n";
                        // auto& itineraryAllotmentQConstraint = functions[itineraryAllotmentQConstraintIndex];
                        // itineraryAllotmentQConstraint = qAllotment - uAllotment * expectedAllotmentShow; // -inf <= itineraryAllotmentQConstraint <= 0
                        cbcLastProblem.coefByVar[qAllotmentIndex].push_back({1, itineraryAllotmentQConstraintIndex});
                        cbcLastProblem.coefByVar[uAllotmentIndex].push_back({-expectedAllotmentShow, itineraryAllotmentQConstraintIndex});
                    }
                }

                // Containers constraint on cutoff
                unsigned cutoffPortConstraintIndex = indexMap.portPositiveCutoffArcConstraints[idBasedArc];
                // auto& cutoffPortConstraint = functions[cutoffPortConstraintIndex];
                // cutoffPortConstraint = containers[idPort]; // inf >= cutoffPortConstraint >= 0
                addMultiVarConstraint(containers[idPort], 1, cutoffPortConstraintIndex, cbcLastProblem.coefByVar);
            } else if (event.type == InputData::Event::Type::arrival) {
                unsigned idNode = arc.toNode;
                const auto& node = input.nodes[idNode];
                const auto& port = input.ports[node.portId];
                auto offhiredIndex = indexMap.timeToSIndex[event.relativeTime];
                // const auto& offhired = variables[indexMap.timeToSIndex[event.relativeTime]];
                // objective -= offhired  * port.offHiringCost;
                cbcLastProblem.objective[offhiredIndex] -= port.offHiringCost;

                // containers[port.id] -= offhired;
                containers[port.id].push_back({-1, offhiredIndex});
                for (unsigned idItinerary : links.itinerariesToArc[idBasedArc]) {
                    // containers[port.id] += takens[idItinerary];
                    for (auto varIndex : takens[idItinerary]) {
                        containers[port.id].push_back({1, varIndex});
                    }
                }

                // Containers constraint on arrival
                unsigned arrivalPortConstraintIndex = indexMap.portPositiveArrivalArcConstraints[idBasedArc];
                // auto& arrivalPortConstraint = functions[arrivalPortConstraintIndex];
                // arrivalPortConstraint = containers[port.id]; // inf >= arrivalPortConstraint >= 0
                addMultiVarConstraint(containers[port.id], 1, arrivalPortConstraintIndex, cbcLastProblem.coefByVar);
            }
        }
        // Common recalculation.
        for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
            const auto& port = input.ports[idPort];
            // objective -= (containers[idPort] * timeDelta) * port.storageCost;
            addMultiVarObjective(containers[idPort], -1 * timeDelta * port.storageCost, cbcLastProblem.objective);
        }
    }

    // Capacity constraints
    for (unsigned idArc = 0; idArc < input.arcs.size(); ++idArc) {
        const auto& arc = input.arcs[idArc];
        if (arc.type == InputData::Arc::Type::travel) {
            // AD<double> inside_arc = 0.0;
            vector<unsigned> inside_arc_indices;
            for (unsigned idItinerary : links.itinerariesWithArc[idArc]) {
                // inside_arc += takens[idItinerary];
                std::copy(takens[idItinerary].begin(), takens[idItinerary].end(), std::back_inserter(inside_arc_indices));
            }
            unsigned constraintIndex = indexMap.arcCapacityConstraints[idArc];
            // AD<double>& constraint = functions[constraintIndex];
            // constraint = inside_arc - input.vessels[arc.vesselId.value()].capacity; // -inf <= constraint  <=0
            addMultiVarConstraint(inside_arc_indices, 1, constraintIndex, cbcLastProblem.coefByVar);
            // constraint = inside_arc // -inf <= constraint <= arc.vessel.capacity
        }
    }

    // Constraints on final container count
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        // double finalCount = input.ports[idPort].finalContainerCount;
        unsigned constraintIndex = indexMap.finalContainerConstraints[idPort];
        // AD<double>& constraint = functions[indexMap.finalContainerConstraints[idPort]];
        // constraint = finalCount - containers[idPort]; // 0 >= constraint >= -INF
        addMultiVarConstraint(containers[idPort], 1, constraintIndex, cbcLastProblem.coefByVar);
        // constraint = containers[idPort] // finalCount <= contraint <= +inf

        // objective -= (-finalCount + containers[idPort]) * (input.ports[idPort].offHiringCost);
        addMultiVarObjective(containers[idPort], -input.ports[idPort].offHiringCost, cbcLastProblem.objective); // TODO: do we need finalCount here?
    }

    // Group constraints
    for (unsigned idGroup = 0; idGroup < input.allotmentGroups.size(); ++idGroup) {
        const auto& group = input.allotmentGroups[idGroup];
        auto& constraintIndex = indexMap.groupConstraints[idGroup];
        vector<unsigned> uAllotmentIndices(group.size());
        std::transform(group.begin(), group.end(), uAllotmentIndices.begin(), [&](unsigned id) { return indexMap.allotmentToUIndex[id]; });

        // for (unsigned idAllotment : group) {
        //     const auto& uAllotment = variables[indexMap.allotmentToUIndex[idAllotment]];
        //     constraint += uAllotment;
        // }
        // // 0 <= constraint <= 1
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

    // Get model.
    // Previously was (seems bad -- gives exception):
    // auto model = solver.getModelPtr();

    CbcModel model(solver);
    model.setIntegerTolerance(config.integerTolerance);
    unsigned cbcLogLevel = config.cbcLogLevel;
    model.setLogLevel(cbcLogLevel);



    if (lastSolution.size() == indexMap.variableCount) {

        // Assertions that no bounds are violated.
        for (std::size_t i = 0; i < indexMap.variableCount; i++) {
            assert(lastSolution[i] <=
                    cbcLastProblem.vupper[i] + CHECK_TOLERANCE);
            assert(lastSolution[i] + CHECK_TOLERANCE >=
                    cbcLastProblem.vlower[i]);
        }

        // Check only if objective looks infeasible.
        bool checkFlag =
            (preparedSolutionObjective > THRESHOLD_SET_OBJ);

        /** User callable setBestSolution.
          If check false does not check valid
          If true then sees if feasible and warns if objective value
          worse than given (so just set to COIN_DBL_MAX
          if you don't care).
          If check true then does not save solution if not feasible
          */

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

    model.setMaximumSeconds(30. * 60);

    // Solve the problem.
    {
        // This code is about redirecting stdout to a file, since
        // no direct hand is provided inside Cbc.

        int fd; fpos_t pos;
        fflush(stdout);
        fgetpos(stdout, &pos);
        fd = dup(fileno(stdout));

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
        dup2(fd, fileno(stdout));
        close(fd);
        clearerr(stdout);
        fsetpos(stdout, &pos);
    }

    auto solution = model.getColSolution();
    double objectiveEstimation = model.getObjValue();
    if (model.isProvenInfeasible() ||
            model.isProvenDualInfeasible()) {
        objectiveEstimation = std::fabs(objectiveEstimation);
    }
    preparedSolutionObjective = objectiveEstimation;

    if (config.stopOnInfeasible) {
        if (model.isProvenInfeasible()
                || model.isProvenDualInfeasible()) {
            return -1;
        }
    }

    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        double finalCount = input.ports[idPort].finalContainerCount;
        objectiveEstimation +=
            finalCount * input.ports[idPort].offHiringCost;
    }

    // Save solution.
    CoinDisjointCopyN(solution, indexMap.variableCount,
            lastSolution.data());

    for (std::size_t i = 0; i < indexMap.variableCount; i++) {
        if (lastSolution[i] > cbcLastProblem.vupper[i]) {
            assert(fabs(lastSolution[i] - cbcLastProblem.vupper[i])
                    < CHECK_TOLERANCE);
        }
        if (lastSolution[i] < cbcLastProblem.vlower[i]) {
            assert(fabs(lastSolution[i] - cbcLastProblem.vlower[i])
                    < CHECK_TOLERANCE);
        }
        lastSolution[i] = std::min(
                cbcLastProblem.vupper[i], lastSolution[i]);
        lastSolution[i] = std::max(
                cbcLastProblem.vlower[i], lastSolution[i]);
    }

    for (const auto& allotment : input.allotments) {
        unsigned allotmentVarId =
            indexMap.allotmentToUIndex[allotment.id];
        lastSolution[allotmentVarId] =
            round(lastSolution[allotmentVarId]);
    }

    return objectiveEstimation;
}


double DetCutPlaneBackend::checkConstraintsAndBounds() {
    double maxViolation = 0.;

    auto& indexMap = indexManager->getConstData();
    vector<double> constrValues(indexMap.constraintCount);
    for (unsigned i = 0; i < cbcLastProblem.coefByVar.size(); i++) {
        for (const CoefIndex& ci : cbcLastProblem.coefByVar[i]) {
            assert(!std::isnan(ci.coef));
            assert(!std::isnan(lastSolution[i]));
            constrValues[ci.index] += lastSolution[i] * ci.coef;
        }
    }

    auto& logger = logging::getBackendSubLogger(BackendType::DET_CUT_PLANE);
    for (unsigned i = 0; i < constrValues.size(); i++) {
        double value = constrValues[i];
        double& lower = cbcLastProblem.glower[i];
        double& upper = cbcLastProblem.gupper[i];
        assert(!std::isnan(lower));
        assert(!std::isnan(upper));
        assert(!std::isnan(value));
        if (value > upper) {
            logger.debugStream() << "Upper constraint violation: (id: "
                << i  << ", value: " << value << ", upper: " << upper << ")";
            maxViolation = std::max(maxViolation, value - upper);
            if (config.tolerateConstraints) {
                upper = value;
            }
        }

        if (value < lower) {
            logger.debugStream() << "Lower constraint violation: (id: " << i
                << ", value: " << value << ", lower: " << lower << ")";
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
            logger.debugStream() << "Lower bound violation: " << " value = " << value
                << " bound = " << lower << " variable_id = " <<  ind;
            maxViolation = std::max(maxViolation, lower - value);
        }
        if (value > upper) {
            logger.debugStream() << "Upper bound violation: " << " value = " << value
                << " bound = " << upper << " variable_id = " <<  ind;
            maxViolation = std::max(maxViolation, value - upper);
        }
    }
    return maxViolation;
}


double DetCutPlaneBackend::calcError() {
    const auto& input = config.inputManager->getConstData();
    const auto& indexMap = indexManager->getConstData();

    double res = 0;
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

                res += fabs(lastSolution[zi] - calcRevenue(
                        demand, lastSolution[demandIndex]));
            }
        }
    }

    return res;
}

void DetCutPlaneBackend::addConstraints() {
    const auto& input = config.inputManager->getConstData();
    auto& indexMap = indexManager->getData();
    indexMap.constraintCount += indexMap.demandZCount;

    cbcLastProblem.reserveNewConstraints(indexMap.demandZCount);

    for (unsigned timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        const auto& event = input.events[timeNow];
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned relatedIndex = 0; relatedIndex < event.relatedItineraryIds.size(); ++relatedIndex) {

                unsigned idItinerary = event.relatedItineraryIds[relatedIndex];

                unsigned demandIndex = indexMap.timeItineraryToDemandIndex[timeNow][idItinerary];

                unsigned zi = indexMap.timeItineraryToZi[timeNow][idItinerary];
                const auto& demand = event.demands[relatedIndex];

                auto demandVal = lastSolution[demandIndex];

                cbcLastProblem.coefByVar[zi].push_back({1, indexMap.constraintCount - indexMap.demandZCount + zi});
                cbcLastProblem.coefByVar[demandIndex].push_back({-calcRevenueDeriv(demand, demandVal),
                    indexMap.constraintCount - indexMap.demandZCount + zi});

                cbcLastProblem.gupper[indexMap.constraintCount - indexMap.demandZCount + zi] =
                    calcRevenue(demand, demandVal) - calcRevenueDeriv(demand, demandVal) * demandVal;

                double oldValue = lastSolution[zi];
                lastSolution[zi] = std::min(lastSolution[zi], calcRevenue(demand, demandVal));
                preparedSolutionObjective += (lastSolution[zi] - oldValue);
            }
        }
    }
}

void DetCutPlaneBackend::genRandomSolution() {
    const auto& input = config.inputManager->getConstData();
    const auto& indexMap = indexManager->getConstData();
    std::default_random_engine gen(config.seed);

    if (lastSolution.size() != size_t(indexMap.variableCount)) {
        lastSolution.assign(indexMap.variableCount, 0);
    }


    for (unsigned timeNow = 0;
            timeNow < input.events.size(); ++timeNow) {
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

                lastSolution[demandIndex] = distribution(gen);
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
                unsigned takeQIndex = indexMap.idItineraryToQIndex[idItinerary];
                double valueQ = floor(lastSolution[takeQIndex] + EPS);
                decision->nonEmptyContainersQ[idItinerary] = unsigned(valueQ);

                // Processing Z_r (setting decision and bounds)
                unsigned emptyZIndex = indexMap.idItineraryToZIndex[idItinerary];
                double valueZ = floor(lastSolution[emptyZIndex] + EPS);
                decision->emptyContainersZ[idItinerary] = unsigned(valueZ);

                // Processing allotments
                for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
                    unsigned takeIQIndex = indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
                    double valueIQ = floor(lastSolution[takeIQIndex] + EPS);
                    unsigned placeIndex = links.allotmentItineraryToPlace.at(idAllotment).at(idItinerary);
                    assert(decision->allotmentContainersQ[idAllotment][placeIndex].first == idItinerary);
                    if (!decision->allotmentAccepted[idAllotment]) {
                        valueIQ = 0;
                    }
                    decision->allotmentContainersQ[idAllotment][placeIndex].second = valueIQ;
                }
            }

            // tracking variables y_a^H
            unsigned hireIndex = indexMap.arcToHired[arc.id];
            if (lastSolution[hireIndex] > 1e-2) {
                decision->hiredY[arc.id] = ceil(lastSolution[hireIndex]);
            }

        } else if (event.type == EventType::pricing) {
            for (unsigned index = 0; index < event.relatedItineraryIds.size(); ++index) {
                unsigned itineraryId = event.relatedItineraryIds[index];
                unsigned variableId = indexMap.timeItineraryToDemandIndex[nextTime][itineraryId];
                double demandValue = solutionValues[variableId];
                double realValue = demandValue;
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
            unsigned variableIndex = indexMap.timeToSIndex[event.relativeTime];
            double offhired = floor(lastSolution[variableIndex] + EPS);
            decision->offHiredInPortS[nextTime][port.id] = offhired;
        } else {
            throw std::logic_error("Event type is not supported");
        }
    }
}

DecisionManagerPtr DetCutPlaneBackend::provideAllotments(double* objectiveValue) {
    auto& logger = logging::getBackendSubLogger(
            BackendType::DET_CUT_PLANE);
    logger.debugStream() << "Entered provideAllotments. ";
    std::string filePath = "decision_" + makeUniqueFileName();
    ManagerConfig decisionConfig =
        {config.needMemory, filePath, true};
    vector<bool> answer;
    logger.debugStream() << "Redirecting to recalculate with empty answer.";

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
    auto debugStream = logger.getStream(log4cpp::Priority::DEBUG);

    double error = 0,
           constraintError = 0,
           accumulatedConstraintError = 0;
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

        logger.getStream(log4cpp::Priority::INFO)
            << "Iteration: " << iterCount << ", " << "Error: " << error << ", "
            << "Constraint error: " << constraintError << ", " << "Accumulated error: "
            << accumulatedConstraintError << ", " << "Objective: " << objectiveEstimation;

        if (objectiveEstimation < 0) {
             break;
        }

    } while (iterCount < config.iterations && error > config.needError);

    const auto& input = config.inputManager->getConstData();

    vector<bool> response(input.allotments.size(), false);
    for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        unsigned u = indexMap.allotmentToUIndex[idAllotment];
        response[idAllotment] = lastSolution[u];
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

    vupper.assign(indexMap.variableCount, INF);
    vupper.shrink_to_fit();
    vlower.assign(indexMap.variableCount, -INF);
    vlower.shrink_to_fit();

    // Q_r, Z_r, Q_ir
    for (unsigned idItinerary = 0; idItinerary < input.itineraries.size(); ++idItinerary) {
        unsigned indexQr = indexMap.idItineraryToQIndex[idItinerary];
        updateLower(vlower[indexQr], double(0.));
        unsigned indexZr = indexMap.idItineraryToZIndex[idItinerary];
        updateLower(vlower[indexZr], double(0.));
        for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
            unsigned indexQir = indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
            updateLower(vlower[indexQir], double(0.));
            unsigned indexEntry = links.allotmentItineraryToEntry.at(idAllotment).at(idItinerary);
            const auto& entry = input.allotmentEntries[indexEntry];
            double expectedShow = entry.productAmount * entry.showRate.estimatedProba;

            // unsigned uAllotmentIndex = indexMap.allotmentToUIndex[idAllotment];
            // const AD<double>& uAllotment = variables[uAllotmentIndex];

            vupper[indexQir] = expectedShow; //TODO do I need it?
        }
    }

    // u_i
    for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        unsigned indexUi = indexMap.allotmentToUIndex[idAllotment];
        updateLower(vlower[indexUi], double(0.0));
        updateUpper(vupper[indexUi], double(1.0));
    }

    // d_t, s_t, y_a^H
    for (const auto& event : input.events) {
        unsigned relativeTime = event.relativeTime;
        // d_t
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned index = 0; index < event.relatedItineraryIds.size(); ++index) {
                unsigned idItinerary = event.relatedItineraryIds[index];
                const auto& demand = event.demands[index];
                unsigned variableIndex = indexMap.timeItineraryToDemandIndex[relativeTime][idItinerary];
                double& lower = vlower[variableIndex];
                double& upper = vupper[variableIndex];

                updateLower(lower, double(0.));
                if (demand.type == Demand::Type::linear) {
                    // updateUpper(upper, demand.additive);
                    updateUpper(upper, demand.additive);
                } else if (demand.type == Demand::Type::exponential) {
                    // updateUpper(upper, demand.scale);
                    updateUpper(upper, demand.scale);
                    updateLower(lower, EXP_BOUND);
                } else {
                    throw std::logic_error("Unsupported demand type");
                }
            }
        // s_t
        } else if (event.type == InputData::Event::Type::arrival) {
            unsigned variableIndex = indexMap.timeToSIndex[relativeTime];
            updateLower(vlower[variableIndex], double(0.));
        // y_a^H
        }  else if (event.type == InputData::Event::Type::cutoff) {
            unsigned basedArcId = event.basedArc.value();
            unsigned variableIndex = indexMap.arcToHired[basedArcId];
            updateLower(vlower[variableIndex], double(0.));
        } else {
            throw std::logic_error("Unsupported event type");
        }
    }

    // z_i bounds
    for (unsigned varIndex = indexMap.variableCount - indexMap.demandZCount; varIndex < indexMap.variableCount; varIndex++) {
        updateLower(vlower[varIndex], double(0.));
    }

    for (unsigned timeNow = 0; timeNow < input.events.size(); ++timeNow) {
        const auto& event = input.events[timeNow];
        if (event.type == InputData::Event::Type::pricing) {
            for (unsigned relatedIndex = 0; relatedIndex < event.relatedItineraryIds.size(); ++relatedIndex) {

                unsigned idItinerary = event.relatedItineraryIds[relatedIndex];

                unsigned zi = indexMap.timeItineraryToZi[timeNow][idItinerary];
                const auto& demand = event.demands[relatedIndex];

                if (demand.type == Demand::Type::linear) {
                    updateUpper(vupper[zi], calcRevenue(demand, demand.additive / 2));
                } else if (demand.type == Demand::Type::exponential) {
                    double optimalDemand = demand.scale * std::exp(-1.0);
                    updateUpper(vupper[zi], calcRevenue(demand, optimalDemand));
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

    glower.assign(indexMap.constraintCount, -INF);
    glower.shrink_to_fit();
    gupper.assign(indexMap.constraintCount, INF);
    gupper.shrink_to_fit();

    // arc capacity constraints
    for (const auto& arc : input.arcs) {
        if (arc.type == InputData::Arc::Type::travel) {
            unsigned constraintIndex = indexMap.arcCapacityConstraints[arc.id];
            updateUpper(gupper[constraintIndex],
                    double(input.vessels[arc.vesselId.value()].capacity * utilizationRatio));
        }
    }

    // positive in ports for cutoff and arrival
    for (const auto& event : input.events) {
        if (event.type == InputData::Event::Type::cutoff) {
            unsigned idBasedArc = event.basedArc.value();
            unsigned constraintIndex = indexMap.portPositiveCutoffArcConstraints[idBasedArc];
            updateLower(glower[constraintIndex], double(0.));
        } else if (event.type == InputData::Event::Type::arrival) {
            unsigned idBasedArc = event.basedArc.value();
            unsigned constraintIndex = indexMap.portPositiveArrivalArcConstraints[idBasedArc];
            updateLower(glower[constraintIndex], double(0.));
        }
    }

    // spotQN constraints
    for (const auto& itinerary : input.itineraries) {
        unsigned idItinerary = itinerary.id;
        unsigned constraintIndex = indexMap.spotQNConstraints[idItinerary];
        updateLower(glower[constraintIndex], double(0.));
    }

    // final container constraints
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        unsigned constraintIndex = indexMap.finalContainerConstraints[idPort];
        double finalCount = input.ports[idPort].finalContainerCount;
        updateUpper(gupper[constraintIndex], finalCount);
    }

    // group constraints
    for (unsigned idGroup = 0; idGroup < input.allotmentGroups.size(); ++idGroup) {
        unsigned constraintIndex = indexMap.groupConstraints[idGroup];
        updateLower(glower[constraintIndex], double(0.));
        updateUpper(gupper[constraintIndex], double(1.));
    }

    for (const auto& allotmentQConstraints : indexMap.allotmentItineraryQConstraints) {
        for (const auto& qConstraintIndex : allotmentQConstraints) {
            updateUpper(gupper[qConstraintIndex], double(0.));
        }
    }
}


} // namespace backend
} // namespace sea

