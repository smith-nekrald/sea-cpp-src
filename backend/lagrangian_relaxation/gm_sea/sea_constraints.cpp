#include "sea_constraints.h"

#include <numeric>
#include <limits>

namespace sea {
namespace backend {

using namespace gm;

void SeaQDescriptor::initBoundsLR(
        vector<double>* vlower, vector<double>* vupper) const {
    const double LOCAL_INF = COIN_DBL_MAX;

    const auto& input = storage.inputManager->getConstData();
    const auto& index = storage.indexManager->getConstData();

    size_t targetLambdaCount = index.lambdaCount;
    size_t targetMuCount = index.muCount;

    size_t ncols = targetLambdaCount + targetMuCount;
    auto& columnLower = *vlower;
    auto& columnUpper = *vupper;

    columnLower.assign(ncols, -LOCAL_INF);
    columnUpper.assign(ncols, LOCAL_INF);
    for (unsigned colId = 0; colId < ncols; ++colId) {
        columnLower[colId] = -LOCAL_INF;
        columnUpper[colId] = LOCAL_INF;
        if (colId < targetLambdaCount) {
            columnLower[colId] = 0;
            columnUpper[colId] = config.maxCapacityCost.value();
        }
    }

    for (const auto& event : input.events) {
        if (event.type == EventType::cutoff) {
            for (unsigned idItinerary : event.relatedItineraryIds) {
                const auto& itinerary = input.itineraries[idItinerary];

                const auto& firstArc = input.arcs[itinerary.orderedArcs.front()];
                const auto& lastArc  = input.arcs[itinerary.orderedArcs.back()];

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
}

void SeaQDescriptor::initConstraintsLR(
        vector<double>* glower, vector<double>* gupper) const {
    const double LOCAL_INF = COIN_DBL_MAX;
    auto& rowLower = *glower;
    auto& rowUpper = *gupper;
    const auto& input = storage.inputManager->getConstData();
    size_t nrows = input.itineraries.size();
    rowLower.assign(nrows, -LOCAL_INF);
    rowUpper.assign(nrows, LOCAL_INF);
    unsigned equationId = 0;
    for (const auto& event : input.events) {
        if (event.type == EventType::cutoff) {
            for (unsigned idItinerary : event.relatedItineraryIds) {
                const auto& itinerary = input.itineraries[idItinerary];
                equationId += 1;
                const auto& firstArc = input.arcs[itinerary.orderedArcs.front()];
                const auto& port =  input.ports[input.nodes[firstArc.fromNode].portId];
                rowLower[equationId - 1] = - itinerary.emptyCost
                    - event.duration * port.storageCost;
            }
        }
    }
}

std::vector<CppAD::AD<double>> SeaQDescriptor::makeConstraints(
        const std::vector<CppAD::AD<double>>& variables) const {
    std::vector<CppAD::AD<double>> response;
    const auto& input = storage.inputManager->getConstData();
    const auto& index = storage.indexManager->getConstData();
    assert(variables.size() == index.lambdaCount + index.muCount);
    for (const auto& event : input.events) {
        if (event.type == EventType::cutoff) {
            for (size_t idItinerary : event.relatedItineraryIds) {
                const auto& itinerary = input.itineraries[idItinerary];
                AD<double> constraint = 0;
                for (auto idArc  : itinerary.orderedArcs) {
                    const auto& arc = input.arcs[idArc];
                    if (arc.type == ArcType::travel) {
                        auto place = index.arcToLambdaIndex[idArc];
                        constraint += variables[place];
                    }
                }
                constraint += variables[itinerary.id + index.lambdaCount];
                response.push_back(constraint);
            }
        }
    }
    return response;
}


} // namespace backend
} // namespace sea
