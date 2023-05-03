#include "functions.h"

#include "../../logging/logging.h"

#include <limits>
#include <filesystem>
#include <stdio.h>

const ui32 MAX_INDEX = std::numeric_limits<ui32>::max();
const double INF = std::numeric_limits<double>::max();


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

    auto& logger = logging::getBackendSubLogger(BackendType::LR);

    {
        logger.infoStream() << "Entered byItineraryRestore. \n " <<
            "Relative time = " << event.relativeTime;
        logger.debug("Duals to backend log: ");
        printDualsToBackendLog(dualVariables, BackendType::LR);
    }

    assert(event.type == EventType::cutoff);
    const auto& input = inputManager->getConstData();
    const auto& magicIndex = indexManager->getConstData();
    const auto& links = linksManager->getConstData();
    auto* decision = decisionManager->get();
    auto* action = &actionManager->getConstData();

    const auto& basedArc = input.arcs[event.basedArc.value()];
    const auto& beginPort = input.ports[input.nodes[basedArc.fromNode].portId];

    vector<ui32> interestingItineraries = event.relatedItineraryIds;
    sort(interestingItineraries.begin(), interestingItineraries.end(),
            [&] (ui32 lhs, ui32 rhs) -> bool {
                return input.itineraries[lhs].declineCost > input.itineraries[rhs].declineCost;
            });
    for(ui32 itineraryId : interestingItineraries) {
        logger.debug("By itinerary restore. Processing itinerary with id"
                + std::to_string(itineraryId));

        vector<double> objective;
        vector<int> correspondingAllotments;

        vector<double> columnLower;
        vector<double> columnUpper;

        const auto& itinerary = input.itineraries[itineraryId];
        ui32 F_r = decision->nonEmptyContainersQ[itinerary.id]
            + decision->emptyContainersZ[itinerary.id];
        logger.debugStream() << "byItineraryRestore: " << "F_r = " << F_r;

        double lambdaSum = 0.;
        for (ui32 idArc : itinerary.orderedArcs) {
            const auto& arc = input.arcs[idArc];
            if (arc.type == ArcType::travel) {
                lambdaSum += dualVariables.lambdaVariables[magicIndex.arcToLambdaIndex[idArc]];
            }
        }
        logger.debugStream() << "byItineraryRestore: " <<  "Lambda Sum = " << lambdaSum;

        double qr = 0, zr = 0;
        qr -= lambdaSum;
        zr -= lambdaSum;
        qr -= itinerary.cost;
        zr -= itinerary.emptyCost;
        qr += itinerary.declineCost;
        qr -= event.duration * input.ports[input.nodes[basedArc.fromNode].portId].storageCost;
        zr -= event.duration * input.ports[input.nodes[basedArc.fromNode].portId].storageCost;

        logger.debugStream() << "byItineraryRestore: " << "qr = " << qr << " zr = " << zr;

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
        correspondingAllotments.push_back(-1);
        columnLower.push_back(0);
        columnUpper.push_back(COIN_DBL_MAX);

        objective.push_back(-yros);
        correspondingAllotments.push_back(-1);
        columnLower.push_back(0);
        columnUpper.push_back(COIN_DBL_MAX);

        for (ui32 allotmentId : links.allotmentsWithItinerary[itinerary.id]) {
            if (decision->allotmentAccepted[allotmentId]) {
                const auto& allotment = input.allotments[allotmentId];
                ui32 placeIndex = links.allotmentItineraryToPlace.at(
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
                ui32 place = links.allotmentItineraryToPlace.at(allotmentId).at(itineraryId);
                columnUpper.push_back(action->allotmentDemandN[allotmentId][place].second);
            } else {
                const auto& allotment = input.allotments[allotmentId];
                ui32 placeIndex =
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
        for (ui32 i = 0; i < rowStart.size(); ++i) {
            rowStart[i] = i * objective.size();
        }
        vector<int> indices(2 * objective.size());
        for (ui32 i = 0; i < 2; ++i) {
            for (ui32 j = 0; j < objective.size(); ++j) {
                indices[i * objective.size() + j] = j;
            }
        }

        {
            auto debugStream  = logger.getStream(log4cpp::Priority::DEBUG);
            debugStream << " Objective: \n";
            for (const auto& val : objective) {
                debugStream << val << " ";
            }
            debugStream << "\n Objective variables: \n";
            debugStream << "qr zr yrhs yros ...\n";
            debugStream << "ColumnLower: " << "\n";
            for (const auto& vl : columnLower) {
                debugStream << vl << " ";
            }
            debugStream << "\nColumnUpper: " << "\n";
            for (const auto& vu: columnUpper) {
                debugStream << vu << " ";
            }

        }

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
        for (ui32 arcId : itinerary.orderedArcs) {
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

        logger.debugStream() << "minCapacityArc = " << minCapacityArc << "\n";

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
            freopen("lr/clp.log", "a+", stdout);
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
        ui32 takenSum = 0;
        decision->emptyContainersZ[itinerary.id] = ui32(solution[1]);
        takenSum += ui32(solution[1]);
        decision->nonEmptyContainersQ[itinerary.id] = ui32(solution[0]);

        logger.debugStream() << "restore.cpp : itinerary = " << itinerary.id;
        logger.debugStream() << "restore.cpp : Q = "
            << decision->nonEmptyContainersQ[itinerary.id];

        takenSum += ui32(solution[0]);

        for (ui32 ind = 0; ind < objective.size(); ++ind) {
            if (correspondingAllotments[ind] != -1) {
                ui32 takenNow = solution[ind];
                takenSum += takenNow;
                ui32 idAllotment = correspondingAllotments[ind];
                ui32 place = links.allotmentItineraryToPlace.at(idAllotment).at(itinerary.id);
                decision->allotmentContainersQ[idAllotment][place].second = takenNow;
                assert(decision->allotmentContainersQ[idAllotment][place].first == itinerary.id);
            }
        }

        ui32 Y_HS = round(solution[2]);
        ui32 Y_OS = round(solution[3]);
        while (F_r + Y_HS < takenSum + Y_OS) {
            if (Y_OS > 0) --Y_OS;
            else ++Y_HS;
        }
        while (F_r + Y_HS > takenSum + Y_OS) {
            if (Y_HS > 0) --Y_HS;
            else ++Y_OS;
        }
        assert(F_r + Y_HS == Y_OS  + takenSum);

        int hiringDiff = int(Y_HS) - int(Y_OS);
        ui32& hiredNow = decision->hiredY[basedArc.id];
        ui32& offHiredNow = decision->offHiredInPortS[event.relativeTime][beginPort.id];
        if (hiringDiff > 0) {
            if (offHiredNow > 0) {
                ui32 minVal = std::min(hiringDiff, int(offHiredNow));
                hiringDiff -= minVal;
                offHiredNow -= minVal;
            }
            hiredNow += hiringDiff;
            decision->offHiredInPortS[links.arrivalTime[itinerary.id]][endPort.id] += hiringDiff;
        } else {
            int offHiringDiff = -hiringDiff;
            if (hiredNow > 0) {
                ui32 minVal = std::min(offHiringDiff, int(hiredNow));
                hiredNow -= minVal;
                offHiringDiff -= minVal;
            }
            decision->offHiredInPortS[event.relativeTime][beginPort.id] += offHiringDiff;;
            ui32 hiringArc = links.firstArcToHireAfterArc[endArc.id];

            if (hiringArc != MAX_INDEX) {
                decision->hiredY[hiringArc] += offHiringDiff;
            }
        }


        state->containersInPorts[beginPort.id] -= takenSum;

        // Update used capacity.
        for (ui32 arcId : itinerary.orderedArcs) {
            const auto& arc = input.arcs[arcId];
            if (arc.type == ArcType::travel) {
                state->usedCapacity[arcId] += takenSum;
                const auto& vessel = input.vessels[arc.vesselId.value()];
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
