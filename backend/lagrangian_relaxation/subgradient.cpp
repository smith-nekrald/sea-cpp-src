// Implements computeSubgradientInPoint method.
//
// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "functions.h"

namespace sea {
namespace backend {

using EventType=InputData::Event::Type;
using ArcType=InputData::Arc::Type;

SubgradientOptimizationParameters computeSubgradientInPoint(
        const DualVariables& point,
        const State& state,
        const ConstLinksManagerPtr& linksManager,
        const ConstInputManagerPtr& inputManager,
        const ConstLRIndexManagerPtr& indexManager,
        DecisionManagerPtr decisionManager,
        UCoefficients* coeffU,
        bool ignoreSpot,
        double l2CoeffReg,
        DualVariables mean) {
    auto& timeParameters = state.timeParameters;
    auto* decision = decisionManager->get();

    SubgradientOptimizationParameters answer = estimateSubgradientWithCppAD(
            point, state, inputManager, linksManager, indexManager,
            decisionManager, ignoreSpot, l2CoeffReg, mean);

    if (coeffU != nullptr) {
        const auto& input = inputManager->getConstData();
        const auto& links = linksManager->getConstData();
        const auto& magicIndex = indexManager->getConstData();
        coeffU->coefficients.assign(input.allotments.size(), 0);
        coeffU->freeMember = 0;

        for (const auto& event: input.events) {
            if (event.type == EventType::cutoff) {
                if (event.relativeTime >= timeParameters.timeEvent) {
                    const auto& basedArc = input.arcs[event.basedArc.value()];
                    for (unsigned itineraryId : event.relatedItineraryIds) {
                        const auto& itinerary = input.itineraries[itineraryId];
                        double lambdaSum = 0.;
                        for (unsigned idArc : itinerary.orderedArcs) {
                            const auto& arc = input.arcs[idArc];
                            if (arc.type == ArcType::travel) {
                                lambdaSum += point.lambdaVariables[
                                    magicIndex.arcToLambdaIndex[idArc]];
                            }
                        }
                        double qr = 0;
                        qr -= lambdaSum;
                        qr -= itinerary.cost;
                        qr += itinerary.declineCost;
                        qr -= event.duration * input.ports[
                            input.nodes[basedArc.fromNode].portId].storageCost;
                        double mu_r = point.muVariables[itinerary.id];

                        for (unsigned allotmentId : links.allotmentsWithItinerary[itinerary.id]) {
                            if (coeffU != nullptr || decision->allotmentAccepted[allotmentId]) {
                                const auto& allotment = input.allotments[allotmentId];
                                unsigned placeIndex = links.allotmentItineraryToPlace.at(
                                        allotment.id).at(itinerary.id);
                                assert(decision->allotmentContainersQ[
                                        allotmentId][placeIndex].first == itinerary.id);
                                // The following assert is OK only when we always change
                                // deterministic solution.
                                assert(decision->allotmentAccepted[allotmentId]
                                        || !decision->allotmentContainersQ[
                                        allotmentId][placeIndex].second);
                                unsigned F_r_i = decision->allotmentContainersQ[
                                    allotmentId][placeIndex].second;
                                double qri = qr;
                                const auto& entry = input.allotmentEntries[
                                    links.allotmentItineraryToEntry.at(
                                            allotment.id).at(itinerary.id)];
                                qri += entry.price;
                                double showMultiplier =
                                    entry.showRate.estimatedProba * entry.productAmount;
                                double objectiveAdd = std::max(0., qri - mu_r);
                                double addFunctionValue = (showMultiplier *
                                        (-itinerary.declineCost - entry.cancellationPrice
                                         + objectiveAdd) + entry.productAmount
                                        * entry.cancellationPrice);
                                addFunctionValue += F_r_i * mu_r;
                                coeffU->coefficients[allotment.id] +=  addFunctionValue;
                            }
                        }
                    }
                }
            }
        }
        coeffU->freeMember = answer.functionValue;
        for (unsigned idAllotment = 0;
                idAllotment < decision->allotmentAccepted.size(); ++idAllotment) {
            if (decision->allotmentAccepted[idAllotment]) {
                coeffU->freeMember -= coeffU->coefficients[idAllotment];
            }
        }
    }
    return answer;
}


} // namespace backend
} // namespace sea
