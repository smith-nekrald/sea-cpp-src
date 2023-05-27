#include "functions.h"
#include "lagrangian_relaxation_backend.h"

#include <limits>
#include<filesystem>
#include <stdio.h>

namespace sea {
namespace backend {

const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();

using ArcType=InputData::Arc::Type;
using EventType=InputData::Event::Type;

void updateBest(std::pair<double, DualVariables>& currentBest,
        const DualVariables& candidate,
        double candidateValue) {
    if (currentBest.first > candidateValue) {
        currentBest.first = candidateValue;
        currentBest.second = candidate;
    }
}

DualVariables shuffleDuals(
        const DualVariables& sourceDuals,
        const std::vector<double>& boundLower,
        const std::vector<double>& boundUpper,
        RandomPack* randomPack,
        bool changeMu,
        bool changeLambda,
        bool useUniform,
        bool useNormal) {
    DualVariables targetDuals = sourceDuals;
    auto& generator = randomPack->generator;
    for (std::size_t idx = 0;
            idx < targetDuals.lambdaVariables.size() + targetDuals.muVariables.size(); ++idx) {
        if (!changeMu && idx >= targetDuals.lambdaVariables.size()) {
            continue;
        }
        if (!changeLambda && idx < targetDuals.lambdaVariables.size()) {
            continue;
        }
        auto& item = (idx < targetDuals.lambdaVariables.size())
            ? targetDuals.lambdaVariables[idx]
            : targetDuals.muVariables[idx - targetDuals.lambdaVariables.size()];
        if (useUniform) {
            item += randomPack->uniformDist(generator);
        }
        if (useNormal) {
            item += randomPack->normalDist(generator);
        }
        item = std::max(item, boundLower[idx]);
        item = std::min(item, boundUpper[idx]);
    }
    return targetDuals;
}

bool checkIfFeasible(
        const DualVariables& variables,
        const std::vector<double>& boundLower,
        const std::vector<double>& boundUpper,
        const std::vector<double>& lhs,
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& configIter) {
    logEntranceToCheckIfFeasible(variables, boundLower, boundUpper, lhs);

    const double FACTOR = 2.;
    const double LOCAL_EPS = FACTOR * configIter.globalPrecision.value();
    const auto& input = inputManager->getConstData();
    assert(variables.muVariables.size() == input.itineraries.size());
    const auto& magicIndex = indexManager->getConstData();

    unsigned lambdaCount = variables.lambdaVariables.size();
    bool ok = true;
    for (unsigned idLambda = 0; ok && (idLambda < lambdaCount); ++idLambda) {
        auto lambdaValue = variables.lambdaVariables[idLambda];
        ok = ok && (lambdaValue + LOCAL_EPS >= boundLower[idLambda])
            && (lambdaValue <= LOCAL_EPS + boundUpper[idLambda]);
    }
    for (unsigned idItinerary = 0; ok && (idItinerary < input.itineraries.size()); ++idItinerary) {
        ok = ok && (variables.muVariables[idItinerary] + LOCAL_EPS
                >= boundLower[idItinerary + lambdaCount]);

        if (!ok) {
            logLowerBoundFailureInCheckIfFeasible(
                    variables.muVariables[idItinerary] - boundLower[idItinerary + lambdaCount]);
            break;
        }
        ok = ok && (variables.muVariables[idItinerary]
                <= LOCAL_EPS + boundUpper[idItinerary + lambdaCount]);
        if (!ok) {
            logUpperBoundFailureInCheckIfFeasible(
                    -variables.muVariables[idItinerary] + boundUpper[idItinerary + lambdaCount]);
            break;
        }

        const auto& itinerary = input.itineraries[idItinerary];
        double sum = variables.muVariables[idItinerary];
        for (unsigned idArc : itinerary.orderedArcs) {
            const auto& arc = input.arcs[idArc];
            if (arc.type == ArcType::travel) {
                unsigned place = magicIndex.arcToLambdaIndex[idArc];
                sum += variables.lambdaVariables[place];
            }
        }
        ok = ok && (sum + LOCAL_EPS >= lhs[itinerary.id]);
        if (!ok) {
            double error = lhs[itinerary.id] - sum;
            logLhsInequalityFailureInCheckIfFeasible(error);
            break;
        }
    }
    auto value = ok ? "true" : "false";
    logExitFromCheckIfFeasible(value);
    return ok;
}

void processMinValue(const std::vector<double> lower,
                     const std::vector<double> upper,
                     DualVariables* target) {
    double minLowerValue = COIN_DBL_MAX;
    for (unsigned idMu = 0; idMu < target->muVariables.size(); ++idMu) {
        unsigned place = idMu + target->lambdaVariables.size();
        minLowerValue = std::min(minLowerValue, lower[place]);
        target->muVariables[idMu] = lower[place];
    }
    assert(upper.size() == lower.size());
    double lambdaBase = std::max(5.0, -minLowerValue);
    target->lambdaVariables.assign(target->lambdaVariables.size(), lambdaBase);
}

void processAvgValue(
        const std::vector<double> lower,
        const std::vector<double> upper,
        DualVariables* target) {
    double minAvgValue = COIN_DBL_MAX;
    for (unsigned idMu = 0; idMu < target->muVariables.size(); ++idMu) {
        unsigned place = idMu + target->lambdaVariables.size();
        minAvgValue = std::min(minAvgValue, 0.5 * (lower[place] + upper[place]));
        target->muVariables[idMu] = lower[place];
    }
    double lambdaBase = std::max(4.0, -minAvgValue);
    target->lambdaVariables.assign(target->lambdaVariables.size(), lambdaBase);
}

void transformAdd(std::bernoulli_distribution& selector,
                  std::default_random_engine& generator,
                  double addValue, DualVariables* target) {
    for (auto& value : target->lambdaVariables) {
        auto decision = selector(generator);
        if (decision) {
            value += addValue;
        }
    }
}

DualVariables provideSomeSolution(
        const std::vector<double>& boundLower,
        const std::vector<double>& boundUpper,
        const std::vector<double>& /*lhs*/,
        const RandomInitType& initType,
        const ConstInputManagerPtr& /*inputManager*/,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& /*config*/,
        RandomPack* randomPack,
        bool minInit,
        double lambdaAdd) {

    auto& generator = randomPack->generator;
    auto& bernoullyDist = randomPack->bernoullyDist;
    auto& normalDist = randomPack->normalDist;
    auto& uniformDist = randomPack->uniformDist;

    DualVariables result;
    initDuals(indexManager->getConstData(), &result);
    if (minInit) {
        processMinValue(boundLower, boundUpper, &result);
    } else {
        processAvgValue(boundLower, boundUpper, &result);
    }
    transformAdd(bernoullyDist, generator, lambdaAdd, &result);

    if (initType == RandomInitType::RANDOM_NORMAL) {
        for (unsigned idMu = 0; idMu < result.muVariables.size(); ++idMu) {
            unsigned place = idMu + result.lambdaVariables.size();
            double shift = fabsl(normalDist(generator));
            result.muVariables[idMu] += shift;
            result.muVariables[idMu] = std::min(result.muVariables[idMu], boundUpper[place]);
        }
        for (unsigned idLambda = 0; idLambda < result.lambdaVariables.size(); ++idLambda) {
            double shift = fabsl(normalDist(generator));
            result.lambdaVariables[idLambda] += shift;
        }

    } else if (initType == RandomInitType::RANDOM_UNIFORM) {
        for (unsigned idMu = 0; idMu < result.muVariables.size(); ++idMu) {
            unsigned place = idMu + result.lambdaVariables.size();
            double shift = fabsl(uniformDist(generator));
            result.muVariables[idMu] += shift;
            result.muVariables[idMu] = std::min(result.muVariables[idMu], boundUpper[place]);
            result.muVariables[idMu] = std::max(result.muVariables[idMu], boundLower[place]);
        }
        for (unsigned idLambda = 0; idLambda < result.lambdaVariables.size(); ++idLambda) {
            double shift = fabsl(uniformDist(generator));
            result.lambdaVariables[idLambda] += shift;
        }
    } else {
        throw std::logic_error("Unknown Init Strategy Type.");
    }
    for (unsigned idLambda = 0; idLambda < result.lambdaVariables.size(); ++idLambda) {
        result.lambdaVariables[idLambda] = std::min(
                boundUpper[idLambda], result.lambdaVariables[idLambda]);
        result.lambdaVariables[idLambda] = std::max(
                boundLower[idLambda], result.lambdaVariables[idLambda]);
    }

    return result;
}

void addDualsToSimplex(
        const DualVariables& point,
        unsigned ncols,
        const LagrangianRelaxationBackendConfig& configIter,
        const State& state,
        const ConstLinksManagerPtr& linksManager,
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        DecisionManagerPtr decisionManager,
        ClpSimplex* simplexBase,
        bool ignoreSpot,
        double* regObjectiveValue,
        DualVariables mean,
        double* trueObjectiveValue) {
    const double LOCAL_INF = COIN_DBL_MAX;
    const double LOCAL_EPS = configIter.zeroIgnorePrecision.value();

    unsigned targetLambdaCount = point.lambdaVariables.size();
    unsigned targetMuCount = point.muVariables.size();

    auto current = computeSubgradientInPoint(point, state,
            linksManager, inputManager, indexManager,
            decisionManager, nullptr, ignoreSpot, configIter.coeffReg.value(), mean);
    if (regObjectiveValue != nullptr) {
        *regObjectiveValue = current.functionValue;
    }
    unsigned rowSize = countNonZeroElements(current, LOCAL_EPS) + 1;
    vector<int> rowIndex(rowSize, 0);
    vector<double> rowValue(rowSize, 0);
    unsigned nextPlace = 0;
    double rowLower = -LOCAL_INF, rowUpper = -current.functionValue;
    for (unsigned idLambda = 0; idLambda < targetLambdaCount; ++idLambda) {
        if (fabs(current.lambdaSubgradient[idLambda]) > LOCAL_EPS) {
            rowIndex[nextPlace] = idLambda;
            rowValue[nextPlace++] = current.lambdaSubgradient[idLambda];
            rowUpper += current.lambdaSubgradient[idLambda] *
                point.lambdaVariables[idLambda];
        }
    }
    for (unsigned idMu = 0; idMu < targetMuCount; ++idMu) {
        if (fabs(current.muSubgradient[idMu]) > LOCAL_EPS) {
            rowIndex[nextPlace] = targetLambdaCount + idMu;
            rowValue[nextPlace++] = current.muSubgradient[idMu];
            rowUpper += current.muSubgradient[idMu] *
                point.muVariables[idMu];
        }
    }
    rowIndex[nextPlace] = ncols - 1;
    rowValue[nextPlace++] = -1.;
    assert(nextPlace == rowSize);
    simplexBase->addRow(rowSize, rowIndex.data(), rowValue.data(), rowLower, rowUpper);
    if (trueObjectiveValue != nullptr) {
        auto pure = computeSubgradientInPoint(point, state,
            linksManager, inputManager, indexManager,
            decisionManager, nullptr, ignoreSpot);
        *trueObjectiveValue = pure.functionValue;
    }
}

void prepareSimplex(
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& configIter,
        DualVariables* dualPtr,
        ClpSimplex* simplexBasePtr,
        vector<double>* columnLowerPtr,
        vector<double>* columnUpperPtr,
        vector<double>* lhsArrayPtr,
        unsigned* ncolsPtr) {

    auto& dualVariables = *dualPtr;
    auto& simplexBase =*simplexBasePtr;

    const auto& input = inputManager->getConstData();
    const auto& magicIndex = indexManager->getConstData();

    // Initialize local constants.
    const double LOCAL_INF = COIN_DBL_MAX;

    // Initialize variables required to solver.
    // Convention: first go lambdaVariables, then muVariables, then artificial variable.
    unsigned targetLambdaCount = dualVariables.lambdaVariables.size();
    unsigned targetMuCount = dualVariables.muVariables.size();

    assert(targetMuCount == input.itineraries.size());
    unsigned& ncols = *ncolsPtr;
    ncols = targetLambdaCount + targetMuCount + 1;
    unsigned nrows = input.itineraries.size();

    vector<double> objective(ncols);
    auto& columnLower = *columnLowerPtr;
    auto& columnUpper = *columnUpperPtr;
    columnLower.assign(ncols, -LOCAL_INF);
    columnUpper.assign(ncols, LOCAL_INF);

    for (unsigned colId = 0; colId < ncols; ++colId) {
        objective[colId] = 0.;
        columnLower[colId] = -LOCAL_INF;
        columnUpper[colId] = LOCAL_INF;
        if (colId < dualVariables.lambdaVariables.size()) {
            columnLower[colId] = 0;
            columnUpper[colId] = configIter.maxCapacityCost.value();
        }
    }
    objective[ncols - 1] = 1;

    simplexBase.setOptimizationDirection(1); // means to minimize
    simplexBase.setLogLevel(configIter.clpLogLevel.value());

    vector<int> rowStart(nrows + 1);
    vector<int> column;
    vector<double> elementByRow, rowLower(nrows, -LOCAL_INF), rowUpper(nrows, LOCAL_INF);

    auto& lhsArray = *lhsArrayPtr;
    lhsArray.assign(input.itineraries.size(), 0);

    unsigned rowPlace = 0;
    unsigned equationId = 0;

    for (const auto& event : input.events) {
        if (event.type == EventType::cutoff) {
            for (unsigned idItinerary : event.relatedItineraryIds) {
                const auto& itinerary = input.itineraries[idItinerary];
                rowStart[equationId++] = rowPlace;

                const auto& firstArc = input.arcs[itinerary.orderedArcs.front()];
                const auto& lastArc  = input.arcs[itinerary.orderedArcs.back()];

                for (unsigned idArc : itinerary.orderedArcs) {
                    const auto& arc = input.arcs[idArc];
                    if (arc.type == ArcType::travel) {
                        unsigned place = magicIndex.arcToLambdaIndex[idArc];
                        assert(place != MAX_INDEX);
                        double value = 1.0;
                        elementByRow.push_back(value);
                        column.push_back(place);
                        ++rowPlace;
                    }
                }
                double coeffMu = 1.0;
                column.push_back(dualVariables.lambdaVariables.size() + itinerary.id);
                elementByRow.push_back(coeffMu);
                ++rowPlace;
                const auto& port =  input.ports[input.nodes[firstArc.fromNode].portId];
                rowLower[equationId - 1] = - itinerary.emptyCost
                    - event.duration * port.storageCost;
                lhsArray[itinerary.id] = rowLower[equationId - 1];
                // Now set bounds on mu.
                const auto& firstNode = input.nodes[firstArc.fromNode];
                const auto& lastNode = input.nodes[lastArc.toNode];
                const auto& firstPort = input.ports[firstNode.portId];
                const auto& lastPort = input.ports[lastNode.portId];
                double k_r_HS = -(firstPort.hiringCost + lastPort.offHiringCost),
                       k_r_OS = -(firstPort.offHiringCost + lastPort.hiringCost);
                columnLower[targetLambdaCount + itinerary.id] =
                    std::max(columnLower[targetLambdaCount + itinerary.id], k_r_OS);
                columnUpper[targetLambdaCount + itinerary.id] =
                    std::min(columnUpper[targetLambdaCount + itinerary.id], -k_r_HS);
            }
        }
    }

    rowStart[equationId++] = rowPlace;
    assert(equationId == nrows + 1);

    CoinPackedMatrix byRow(false, ncols, nrows, elementByRow.size(),
            elementByRow.data(), column.data(), rowStart.data(), nullptr) ;

    simplexBase.loadProblem(byRow, columnLower.data(), columnUpper.data(),
            objective.data(), rowLower.data(), rowUpper.data());
}

std::pair<double, DualVariables> initializeCuttingPlane(
        const ConstInputManagerPtr& inputManager,
        const ConstLinksManagerPtr& linksManager,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& configIter,
        const State& state,
        const vector<double>& columnLower,
        const vector<double>& columnUpper,
        const vector<double>& lhsArray,
        unsigned ncols,
        RandomPack* randomPack,
        std::deque<DualDequeInfo>* dualHistory,
        ClpSimplex& simplexBase,
        DecisionManagerPtr& decisionManager,
        bool ignoreSpot,
        DualVariables mean) {

    std::pair<double, DualVariables> response = {COIN_DBL_MAX, DualVariables()};
    double regObjectiveHolder = COIN_DBL_MAX;
    double trueObjectiveHolder = COIN_DBL_MAX;

    const double UPPER_SHIFT = configIter.initCapacityCost.value();

    const vector<bool> flagVals = {true, false};
    const vector<double> shiftVals = {0, UPPER_SHIFT};
    for (unsigned idPoint = 0; idPoint < configIter.initPointCount.value(); ++idPoint) {
        DualVariables point;
        vector<RandomInitType> initTypes;
        if (configIter.initStrategy == RandomInitType::RANDOM_BOTH
                || configIter.initStrategy == RandomInitType::RANDOM_NORMAL) {
            initTypes.push_back(RandomInitType::RANDOM_NORMAL);
        }
        if (configIter.initStrategy == RandomInitType::RANDOM_UNIFORM
                || configIter.initStrategy == RandomInitType::RANDOM_BOTH) {
            initTypes.push_back(RandomInitType::RANDOM_UNIFORM);
        }
        for (auto flag : flagVals) {
            for (auto shift : shiftVals) {
                for (auto init : initTypes) {
                    point = provideSomeSolution(columnLower, columnUpper,
                            lhsArray, init,
                            inputManager, indexManager,
                            configIter, randomPack,
                            flag, shift);
                    assert(point.muVariables.size()
                            == inputManager->getConstData().itineraries.size());
                    if (checkIfFeasible(point, columnLower, columnUpper, lhsArray,
                                inputManager, indexManager, configIter)) {
                        addDualsToSimplex(point, ncols,
                                configIter, state,
                                linksManager, inputManager,
                                indexManager, decisionManager,
                                &simplexBase, ignoreSpot, &regObjectiveHolder,
                                mean, &trueObjectiveHolder);
                        updateBest(response, point, trueObjectiveHolder);
                    }
                }
            }
        }
    }
    for (auto& info : *dualHistory) {
        auto& point = info.duals;
        bool checkResult = info.checked;
        bool writeLogOut = false;
        if (info.immortal && !info.checked) {
            logProcessingImmortalUnchecked();
            writeLogOut = true;
        }
        if (!info.checked) {
            checkResult = checkIfFeasible(
                point, columnLower, columnUpper,
                lhsArray, inputManager, indexManager, configIter);
        }

        for (std::size_t idMu = 0; (!checkResult) &&
                idMu < point.muVariables.size(); ++idMu) {
            point.muVariables[idMu] =
                columnUpper[idMu + point.lambdaVariables.size()];
        }

        if (checkResult ||
                checkIfFeasible(point, columnLower, columnUpper,
                lhsArray, inputManager, indexManager, configIter)) {
            info.checked = true;
            addDualsToSimplex(point, ncols,
                    configIter, state,
                    linksManager, inputManager,
                    indexManager, decisionManager,
                    &simplexBase, ignoreSpot,
                    &regObjectiveHolder, mean, &trueObjectiveHolder);
            updateBest(response, point, trueObjectiveHolder);
        }
        if (info.immortal) {
            logImmortalNorms(info);
            auto boolArray = {true, false};
            for (std::size_t idx = 0;
                    idx < configIter.immortalShuffleCount; ++idx) {
                for (const auto& useNormal : boolArray) {
                    for (const auto& useUniform : boolArray) {
                        for (const auto& changeMu : boolArray) {
                            for (const auto& changeLambda : boolArray) {
                                auto duals = shuffleDuals(info.duals,
                                        columnLower, columnUpper, randomPack,
                                        changeMu, changeLambda, useUniform, useNormal);
                                if (checkIfFeasible(duals, columnLower, columnUpper, lhsArray,
                                            inputManager, indexManager, configIter)) {
                                    addDualsToSimplex(duals, ncols,
                                            configIter, state,
                                            linksManager, inputManager,
                                            indexManager, decisionManager,
                                            &simplexBase, ignoreSpot, &regObjectiveHolder,
                                            mean, &trueObjectiveHolder);
                                    updateBest(response, duals, trueObjectiveHolder);
                                }
                            }
                        }
                    }
                }
            }
        }
        logImmortalProcessed(writeLogOut, info);
    }
    return response;
}

size_t computeTrueCount(const vector<bool>& hitVector) {
    size_t count = 0;
    for (auto hit : hitVector) {
        if (hit) {
            ++count;
        }
    }
    return count;
}

size_t computeHitDiff(const vector<bool>& prevVector, const vector<bool>& newVector) {
    assert(prevVector.size() == newVector.size());
    size_t count = 0;
    for (size_t idx = 0; idx < prevVector.size(); ++idx) {
        if (prevVector[idx] != newVector[idx]) {
            ++count;
        }
    }
    return count;
}

pair<size_t, size_t> computeHitStats(const DualTemplate<bool>& currentHit) {
    size_t hitLambda = computeTrueCount(currentHit.lambdaVariables),
           hitMu = computeTrueCount(currentHit.muVariables);
    return {hitLambda, hitMu};
}

pair<size_t, size_t> computeHitChange(
        const DualTemplate<bool>& prevHit, const DualTemplate<bool>& newHit) {
    size_t changeLambda = computeHitDiff(prevHit.lambdaVariables,
                newHit.lambdaVariables),
           changeMu = computeHitDiff(prevHit.muVariables,
                newHit.muVariables);
    return {changeLambda, changeMu};
}

void computeBoundHit(
        const vector<double>& bound,
        const DualVariables& dualVariables,
        DualTemplate<bool>* hitMap) {
    const double EPS = 1e-3;
    assert(hitMap != nullptr);
    size_t targetLambdaCount = dualVariables.lambdaVariables.size();
    size_t targetMuCount = dualVariables.muVariables.size();
    if (hitMap->lambdaVariables.size() != targetLambdaCount) {
        hitMap->lambdaVariables.assign(targetLambdaCount, false);
    }
    if (hitMap->muVariables.size() != targetMuCount) {
        hitMap->muVariables.assign(targetMuCount, false);
    }
    vector<int> hitMu, hitLambda;
    for (size_t idx = 0; idx < targetLambdaCount; ++idx) {
        if (fabsl(dualVariables.lambdaVariables[idx]
                    - bound[idx]) < EPS) {
            hitMap->lambdaVariables[idx] = true;
            hitLambda.push_back(idx);
        } else {
            hitMap->lambdaVariables[idx] = false;
        }
    }
    for (size_t idx = 0; idx < targetMuCount; ++idx) {
        if (fabsl(dualVariables.muVariables[idx]
                    - bound[idx + targetLambdaCount]) < EPS) {
            hitMu.push_back(idx);
            hitMap->muVariables[idx] = true;
        } else {
            hitMap->muVariables[idx] = false;
        }
    }
    logHitMuAndLambda(hitMu, hitLambda);
}

vector<bool> computePlaneHits(
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        const vector<double>& lhs,
        const DualVariables& variables) {
    vector<int> hitIdx;
    const auto& input = inputManager->getConstData();
    const auto& magicIndex = indexManager->getConstData();
    const double EPS = 1e-3;
    vector<bool> response(input.itineraries.size(), false);
    for (size_t idItinerary = 0; idItinerary < input.itineraries.size(); ++idItinerary) {
        const auto& itinerary = input.itineraries[idItinerary];
        double sum = variables.muVariables[idItinerary];
        for (unsigned idArc : itinerary.orderedArcs) {
            const auto& arc = input.arcs[idArc];
            if (arc.type == ArcType::travel) {
                unsigned place = magicIndex.arcToLambdaIndex[idArc];
                sum += variables.lambdaVariables[place];
            }
        }
        if (fabsl(lhs[itinerary.id] - sum) < EPS) {
            response[itinerary.id] = true;
            hitIdx.push_back(itinerary.id);
        }
    }
    logPlaneIntersectionItineraries(hitIdx);
    return response;
}

void doCuttingPlaneOptimization(
        const ConstInputManagerPtr& inputManager,
        const ConstLinksManagerPtr& linksManager,
        const ConstLRIndexManagerPtr& indexManager,
        const LagrangianRelaxationBackendConfig& configIter,
        const State& state,
        DecisionManagerPtr& decisionManager,
        RandomPack* randomPack,
        DualVariables* dualPtr,
        std::deque<DualDequeInfo>* dualHistory,
        double* estimatedObjective,
        UCoefficients* uCoeff,
        bool ignoreSpot,
        DualVariables mean) {

    const double LOCAL_INF = COIN_DBL_MAX;
    const double LOCAL_EPS = configIter.globalPrecision.value();
    auto& dualVariables = *dualPtr;
    size_t targetLambdaCount = dualVariables.lambdaVariables.size();
    size_t targetMuCount = dualVariables.muVariables.size();

    logStartCuttingPlaneOptimization(state, dualPtr);

    ClpSimplex simplexBase;

    vector<double> columnLower, columnUpper, lhsArray;
    unsigned ncols = 0;
    prepareSimplex(inputManager, indexManager, configIter, dualPtr,
        &simplexBase, &columnLower, &columnUpper, &lhsArray, &ncols);
    auto bestPoint = initializeCuttingPlane(
        inputManager, linksManager, indexManager,
        configIter, state, columnLower, columnUpper, lhsArray,
        ncols, randomPack, dualHistory, simplexBase, decisionManager,
        ignoreSpot, mean);
    auto prevDuals = bestPoint.second;
    double lastObjective = -LOCAL_INF;
    std::size_t restartCount = 0;

    DualTemplate<bool> prevLowerBoundHit, prevUpperBoundHit;
    computeBoundHit(columnLower, prevDuals, &prevLowerBoundHit);
    computeBoundHit(columnUpper, prevDuals, &prevUpperBoundHit);
    vector<bool> prevPlaneHits;
    prevPlaneHits = computePlaneHits(inputManager, indexManager, lhsArray, prevDuals);

    for (unsigned iter = 0;
            iter < configIter.maxSubgradientIterations.value(); ++iter) {
        ClpSimplex simplexFinal(simplexBase);

        // Optimization in a separate block to ensure correct logging to clp.log.
        {
            int fd; fpos_t pos;
            fflush(stdout);
            fgetpos(stdout, &pos);
            fd = dup(fileno(stdout));
            auto folderPath = std::filesystem::path("lr");
            if (!std::filesystem::exists(folderPath)) {
                std::filesystem::create_directories(folderPath);
            }
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wunused-result"
            freopen("lr/clp.log", "a+", stdout);
            #pragma GCC diagnostic pop
            printf("LR-cutting-plane. Started computation.\n");
            if (configIter.clpMethod == ClpSolutionConfig::CLP_PRIMAL) {
                printf("LR-cutting-plane. Primal method.\n");
                simplexFinal.primal();
            } else if (configIter.clpMethod == ClpSolutionConfig::CLP_DUAL) {
                printf("LR-cutting-plane. Dual method.\n");
                simplexFinal.dual();
            }
            printf("LR-cutting-plane. Computation finished.\n");
            fflush(stdout);
            dup2(fd, fileno(stdout));
            close(fd);
            clearerr(stdout);
            fsetpos(stdout, &pos);
        }

        double *primal = simplexFinal.primalColumnSolution();
        // This is the way to get dual solution:
        // double *dual = simplexFinal.dualRowSolution();

        for (unsigned idLambda = 0; idLambda < targetLambdaCount; ++idLambda) {
            dualVariables.lambdaVariables[idLambda] = primal[idLambda];
        }
        for (unsigned idMu = 0; idMu < targetMuCount; ++idMu) {
            dualVariables.muVariables[idMu] = primal[targetLambdaCount + idMu];
        }
        double cuttingPlaneObjective = simplexFinal.getObjValue();
        bool feasible = checkIfFeasible(dualVariables, columnLower,
                columnUpper, lhsArray,
                inputManager, indexManager, configIter);
        logCuttingPlaneIterationDuals(iter, dualVariables, feasible);

        double regObjectiveValue = -1e100;
        double trueObjectiveValue = -1e100;
        addDualsToSimplex(
                dualVariables, ncols,
                configIter, state,
                linksManager, inputManager,
                indexManager, decisionManager,
                &simplexBase, ignoreSpot,
                &regObjectiveValue, mean, &trueObjectiveValue);
        updateBest(bestPoint, dualVariables, trueObjectiveValue);
        DualDequeInfo info;
        info.duals = dualVariables;
        info.immortal = false;
        info.checked = true;
        dualHistory->push_back(info);
        while (dualHistory->size() > configIter.deque_size) {
            if (dualHistory->front().immortal) {
                assert(dualHistory->size()
                    >= configIter.immortalLimit);
                dualHistory->push_back(dualHistory->front());
            }
            dualHistory->pop_front();
        }
        logDualHistorySize(dualHistory->size());

        bool canStopShift = false, canStopIter = false;
        canStopIter = (iter >= configIter.minSubgradientIterations.value());
        double absObjectiveDiff = fabsl(cuttingPlaneObjective - lastObjective);

        logObjectiveAndDualNorms(
                cuttingPlaneObjective, regObjectiveValue, dualVariables, prevDuals);
        auto currentLowerBoundHit = prevLowerBoundHit;
        auto currentUpperBoundHit = prevUpperBoundHit;
        computeBoundHit(columnLower, dualVariables, &currentLowerBoundHit);
        computeBoundHit(columnUpper, dualVariables, &currentUpperBoundHit);

        vector<bool> currentPlaneHits =  computePlaneHits(
            inputManager, indexManager, lhsArray, dualVariables);

        auto upperHitCount = computeHitStats(currentUpperBoundHit);
        auto lowerHitCount = computeHitStats(currentLowerBoundHit);

        auto lowerChange = computeHitChange(currentLowerBoundHit, prevLowerBoundHit);
        auto upperChange = computeHitChange(currentUpperBoundHit, prevUpperBoundHit);

        logChangeInCuttingPlane(lowerChange, upperChange, upperHitCount, lowerHitCount,
                targetMuCount, targetLambdaCount, prevPlaneHits, currentPlaneHits);

        prevLowerBoundHit = currentLowerBoundHit;
        prevUpperBoundHit = currentUpperBoundHit;
        prevPlaneHits = currentPlaneHits;

        prevDuals = dualVariables;
        double absShiftDiff = fabsl(regObjectiveValue - cuttingPlaneObjective);
        double relShiftDiff = absShiftDiff /
            (0.5 * (fabsl(regObjectiveValue) + fabsl(cuttingPlaneObjective)));
        canStopShift = (relShiftDiff < LOCAL_EPS);
        if (canStopShift && canStopIter) {
            logLastCuttingPlaneIteration(iter);
            break;
        }
        lastObjective = cuttingPlaneObjective;

        if (configIter.useEquationsRestart && (iter > configIter.deque_size.value()) &&
                (iter % configIter.restartPeriod.value() ==
                    (configIter.restartPeriod.value() - 1))) {
            if (!configIter.singleRestart || !restartCount) {
                assert(configIter.deque_size.value()
                        >= configIter.restartPeriod.value() / 3.);
                logRestartSimplexFromDeque();
                ClpSimplex newSimplexBase;
                prepareSimplex(inputManager, indexManager, configIter,
                        dualPtr, &newSimplexBase, &columnLower,
                        &columnUpper, &lhsArray, &ncols);
                simplexBase = newSimplexBase;
                auto newBest = initializeCuttingPlane(
                        inputManager, linksManager,
                        indexManager, configIter, state, columnLower,
                        columnUpper, lhsArray, ncols, randomPack,
                        dualHistory, simplexBase, decisionManager,
                        ignoreSpot, mean);
                if (newBest.first < bestPoint.first) {
                    bestPoint = newBest;
                }
                restartCount += 1;
            }
        }
        logFinishedCuttingPlaneIteration(iter, absObjectiveDiff, absShiftDiff, relShiftDiff);
    }
    dualVariables = bestPoint.second;
    auto current = computeSubgradientInPoint(
            dualVariables, state, linksManager, inputManager, indexManager, decisionManager,
            uCoeff, ignoreSpot, configIter.coeffReg.value(), mean);
    logSubgradient(current);

    if (estimatedObjective != nullptr) {
        *estimatedObjective = current.functionValue;
    }
}

} // namespace backend
} // namespace sea
