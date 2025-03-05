// Implements byItineraryRestore - method to make allocation decisions at cut-off events.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
// (c) Smith School of Business, 2025

#include "functions.h"

#include <limits>
#include <filesystem>
#include <cmath>
#include <stdio.h>
#include <cassert>


namespace sea {
namespace backend {

using EventType=InputData::Event::Type;
using ArcType=InputData::Arc::Type;

void byItineraryRestore(
        const InputData::Event& event,
        const ConstInputManagerPtr& inputManager,
        const ConstLinksManagerPtr& linksManager,
        const ConstLRIndexManagerPtr& indexManager,
        const ConstActionManagerPtr& actionManager,
        const DualVariables& dualVariables,
        State* state,
        DecisionManagerPtr decisionManager) {
    logEnteredByItineraryRestore(event, dualVariables);

    assert(event.type == EventType::cutoff);
    const auto& input = inputManager->getConstData();
    const auto& lrIndex = indexManager->getConstData();
    const auto& links = linksManager->getConstData();
    auto* decision = decisionManager->get();
    auto* action = &actionManager->getConstData();

    const auto& basedArc = input.arcs[event.basedArc.value()];
    const auto& beginPort = input.ports[input.nodes[basedArc.fromNode].portId];

    vector<unsigned> interestingItineraries = event.relatedItineraryIds;
    sort(interestingItineraries.begin(), interestingItineraries.end(),
            [&] (unsigned lhs, unsigned rhs) -> bool {
                return input.itineraries[lhs].declineCost > input.itineraries[rhs].declineCost;
            });
    for(auto itineraryId : interestingItineraries) {
        vector<double> objective;
        vector<int> correspondingAllotments;

        vector<double> columnLower;
        vector<double> columnUpper;

        const auto& itinerary = input.itineraries[itineraryId];
        unsigned F_r = decision->nonEmptyContainersQ[itinerary.id]
            + decision->emptyContainersZ[itinerary.id];

        double lambdaSum = 0.;
        for (unsigned idArc : itinerary.orderedArcs) {
            const auto& arc = input.arcs[idArc];
            if (arc.type == ArcType::travel) {
                lambdaSum += dualVariables.lambdaVariables[lrIndex.arcToLambdaIndex[idArc]];
            }
        }

        double qr = 0, zr = 0;
        qr -= lambdaSum;
        zr -= lambdaSum;
        qr -= itinerary.cost;
        zr -= itinerary.emptyCost;
        qr += itinerary.declineCost;
        qr -= event.duration * input.ports[input.nodes[basedArc.fromNode].portId].storageCost;
        zr -= event.duration * input.ports[input.nodes[basedArc.fromNode].portId].storageCost;

        logProcessingItineraryInByItineraryRestore(itineraryId, lambdaSum, qr, zr);

        objective.push_back(qr);
        correspondingAllotments.push_back(-1);
        columnLower.push_back(0);
        columnUpper.push_back(action->spotMarketDemandN[itineraryId]);

        objective.push_back(zr);
        correspondingAllotments.push_back(-1);
        columnLower.push_back(0);
        columnUpper.push_back(COIN_DBL_MAX);

        const auto& endArc = input.arcs[itinerary.orderedArcs.back()];
        const auto& endPort = input.ports[input.nodes[endArc.toNode].portId];

        double yrhs = beginPort.hiringCost + endPort.offHiringCost;
        double yros = beginPort.offHiringCost + endPort.hiringCost;

        objective.push_back(-yrhs);
        // To indicate that this index is irrelevant for allotments.
        correspondingAllotments.push_back(-1);
        columnLower.push_back(0);
        columnUpper.push_back(COIN_DBL_MAX);

        objective.push_back(-yros);
        // To indicate that this index is irrelevant for allotments.
        correspondingAllotments.push_back(-1);
        columnLower.push_back(0);
        columnUpper.push_back(COIN_DBL_MAX);

        for (unsigned allotmentId : links.allotmentsWithItinerary[itinerary.id]) {
            if (decision->allotmentAccepted[allotmentId]) {
                const auto& allotment = input.allotments[allotmentId];
                unsigned placeIndex = links.allotmentItineraryToPlace.at(
                        allotment.id).at(itinerary.id);
                assert(decision->allotmentContainersQ[allotmentId][placeIndex].first
                        == itinerary.id);
                // The following assert is OK only when we always change deterministic solution.
                F_r += decision->allotmentContainersQ[allotmentId][placeIndex].second;
                double qri = qr;
                const auto& entry = input.allotmentEntries[links.allotmentItineraryToEntry
                    .at(allotment.id).at(itinerary.id)];
                qri += entry.price;

                objective.push_back(qri);
                correspondingAllotments.push_back(allotmentId);
                columnLower.push_back(0);
                unsigned place = links.allotmentItineraryToPlace.at(allotmentId).at(itineraryId);
                columnUpper.push_back(action->allotmentDemandN[allotmentId][place].second);
            } else {
                const auto& allotment = input.allotments[allotmentId];
                unsigned placeIndex =
                    links.allotmentItineraryToPlace.at(allotment.id).at(itinerary.id);
                assert(decision->allotmentContainersQ[allotmentId][placeIndex].first
                    == itinerary.id);
                decision->allotmentContainersQ[allotment.id][placeIndex].second = 0;
            }
        }


        vector<double> rowValue(2 * objective.size(), 1);
        rowValue[2] = -1;   // yr_HS in first row
        rowValue[objective.size() + 2] = 0;    // yr_HS in second row
        rowValue[objective.size() + 3] = 0;    // yr_OS in second row

        vector<int> rowStart(3, 0);
        for (unsigned i = 0; i < rowStart.size(); ++i) {
            rowStart[i] = i * objective.size();
        }
        vector<int> indices(2 * objective.size());
        for (unsigned i = 0; i < 2; ++i) {
            for (unsigned j = 0; j < objective.size(); ++j) {
                indices[i * objective.size() + j] = j;
            }
        }

        logObjectiveAndBoundsInByItineraryRestore(objective, columnLower, columnUpper);

        CoinPackedMatrix byRow(false,               //row ordered
                               objective.size(),    //number of columns
                               2,                   //number of rows
                               rowValue.size(),     //number of elements in matrix
                               rowValue.data(),
                               indices.data(),
                               rowStart.data(),
                               nullptr);

        vector<double> rowLower(2, 0);
        vector<double> rowUpper(2, 0);

        rowLower[0] = F_r;
        rowUpper[0] = F_r;

        double minCapacityArc = COIN_DBL_MAX;
        for (unsigned arcId : itinerary.orderedArcs) {
            const auto& innerArc = input.arcs[arcId];
            if (innerArc.type == ArcType::travel) {
                const auto& vessel = input.vessels[innerArc.vesselId.value()];
                double allowToUse = vessel.capacity - state->usedCapacity[arcId];
                assert(allowToUse >= 0);
                minCapacityArc = std::min<double>(minCapacityArc, allowToUse);
            }
        }

        rowLower[1] = 0;
        rowUpper[1] = minCapacityArc;

        logMinCapacityArc(minCapacityArc);

        ClpSimplex solver;
        solver.setOptimizationDirection(-1);    // -> max

        solver.loadProblem(byRow, columnLower.data(), columnUpper.data(),
                objective.data(), rowLower.data(), rowUpper.data());

        // solve
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
            printf("LR-restore. Started computation.\n");

            solver.primal();

            printf("LR-restore. Finished computation.\n");
            fflush(stdout);
            dup2(fd, fileno(stdout));
            close(fd);
            clearerr(stdout);
            fsetpos(stdout, &pos);

        }
        const double* solution = solver.primalColumnSolution();

        // Reconstructing decision from solution.
        unsigned shippedSum = 0;

        const double FLOOR_EPS = 1e-3;
        double emptyCount = floor(solution[1] + FLOOR_EPS);
        assert(emptyCount >= 0.);
        decision->emptyContainersZ[itinerary.id] = static_cast<unsigned>(emptyCount);
        shippedSum += static_cast<unsigned>(emptyCount);

        double nonEmptySpotCount = floor(solution[0] + FLOOR_EPS);
        assert(nonEmptySpotCount >= 0.);
        decision->nonEmptyContainersQ[itinerary.id] = static_cast<unsigned>(nonEmptySpotCount);
        shippedSum += static_cast<unsigned>(nonEmptySpotCount);

        logNonEmptyQInByItineraryRestore(
                itinerary.id, decision->nonEmptyContainersQ[itinerary.id]);

        for (unsigned ind = 0; ind < objective.size(); ++ind) {
            // Otherwise, this entry in the solution is irrelevant for allotments.
            if (correspondingAllotments[ind] != -1) {
                [[maybe_unused]] double longCount = floor(solution[ind] + FLOOR_EPS);
                assert(longCount >= 0);
                unsigned loadNow = static_cast<unsigned> (solution[ind]);
                shippedSum += loadNow;
                unsigned idAllotment = correspondingAllotments[ind];
                unsigned place = links.allotmentItineraryToPlace.at(
                    idAllotment).at(itinerary.id);
                decision->allotmentContainersQ[idAllotment][place].second = loadNow;
                assert(decision->allotmentContainersQ[idAllotment][place].first == itinerary.id);
            }
        }

        // Rebalancing hiring-offhiring-allocating for containers.
        double Y_HS_solution = round(solution[2]), Y_OS_solution = round(solution[3]);
        assert(Y_HS_solution >= 0. && Y_OS_solution >= 0.);
        unsigned Y_HS = static_cast<unsigned>(Y_HS_solution);
        unsigned Y_OS = static_cast<unsigned>(Y_OS_solution);
        while (F_r + Y_HS < shippedSum + Y_OS) {
            if (Y_OS > 0) {
                --Y_OS;
            } else {
                ++Y_HS;
            }
        }
        while (F_r + Y_HS > shippedSum + Y_OS) {
            if (Y_HS > 0) {
                --Y_HS;
            } else {
                ++Y_OS;
            }
        }
        assert(F_r + Y_HS == Y_OS  + shippedSum);

        int hiringDiff = static_cast<int>(Y_HS) - static_cast<int>(Y_OS);
        unsigned & hiredNow = decision->hiredY[basedArc.id];
        unsigned & offHiredNow = decision->offHiredInPortS[event.relativeTime][beginPort.id];
        if (hiringDiff > 0) {
            if (offHiredNow > 0) {
                int minVal = std::min(hiringDiff, static_cast<int>(offHiredNow));
                assert(minVal >= 0);
                hiringDiff -= minVal;
                offHiredNow -= minVal;
            }
            hiredNow += hiringDiff;
            decision->offHiredInPortS[links.arrivalTime[itinerary.id]][endPort.id] += hiringDiff;
        } else {
            int offHiringDiff = -hiringDiff;
            if (hiredNow > 0) {
                int minVal = std::min(offHiringDiff, static_cast<int>(hiredNow));
                assert(minVal >= 0);
                hiredNow -= minVal;
                offHiringDiff -= minVal;
            }
            decision->offHiredInPortS[event.relativeTime][beginPort.id] += offHiringDiff;;
            unsigned hiringArc = links.firstArcToHireAfterArc[endArc.id];

            const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();
            if (hiringArc != MAX_INDEX) {
                decision->hiredY[hiringArc] += offHiringDiff;
            }
        }
        state->containersInPorts[beginPort.id] -= shippedSum;
        // Update used capacity.
        for (unsigned arcId : itinerary.orderedArcs) {
            const auto& arc = input.arcs[arcId];
            if (arc.type == ArcType::travel) {
                state->usedCapacity[arcId] += shippedSum;
                [[maybe_unused]] const auto& vessel = input.vessels[arc.vesselId.value()];
                assert(state->usedCapacity[arcId] <= vessel.capacity);
            }
        }
    }
    state->containersInPorts[beginPort.id] += decision->hiredY[basedArc.id];
    if (state->containersInPorts[beginPort.id] < 0) {
        decision->hiredY[basedArc.id] += (-state->containersInPorts[beginPort.id]);
        state->containersInPorts[beginPort.id] = 0;
    }
}


} // namespace backend
} // namespace sea

