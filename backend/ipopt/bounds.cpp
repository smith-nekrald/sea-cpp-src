// API computing variable and constraint bounds for fluid approximation optimization via Ipopt.

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

namespace sea {
namespace backend {

using std::unordered_map;
using EventType = InputData::Event::Type;
using ArcType = InputData::Arc::Type;
using std::cout;
using std::endl;
using std::size_t;


void IpoptBackend::initBoundsLR(vector<double>* vlowerPtr, vector<double>* vupperPtr) const {
    const auto& input = config.inputManager->getConstData();
    const auto& links = config.linksManager->getConstData();
    const auto& indexMap = indexManager->getConstData();
    assert(vupperPtr != nullptr && vlowerPtr != nullptr);

    auto& vupper = *vupperPtr;
    auto& vlower = *vlowerPtr;

    const double INF = std::numeric_limits<double>::max();
    const double ZERO = 0.;
    const double ONE = 1.;

    vupper.assign(indexMap.variableCount, INF);
    vupper.shrink_to_fit();
    vlower.assign(indexMap.variableCount, -INF);
    vlower.shrink_to_fit();

    // Q_r, Z_r, Q_ir
    for (unsigned idItinerary = 0; idItinerary < input.itineraries.size(); ++idItinerary) {
        unsigned indexQr = indexMap.idItineraryToQIndex[idItinerary];
        updateLower(vlower[indexQr], ZERO);
        unsigned indexZr = indexMap.idItineraryToZIndex[idItinerary];
        updateLower(vlower[indexZr], ZERO);
        for (unsigned idAllotment : links.allotmentsWithItinerary[idItinerary]) {
            unsigned indexQir = indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
            updateLower(vlower[indexQir], ZERO);
            unsigned indexEntry = links.allotmentItineraryToEntry.at(idAllotment).at(idItinerary);
            const auto& entry = input.allotmentEntries[indexEntry];
            double expectedShow = entry.productAmount * entry.showRate.estimatedProba;
            updateUpper(vupper[indexQir], expectedShow);
        }
    }

    // u_i
    for (unsigned idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        unsigned indexUi = indexMap.allotmentToUIndex[idAllotment];
        updateLower(vlower[indexUi], ZERO);
        updateUpper(vupper[indexUi], ONE);
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

                updateLower(lower, ZERO);
                if (demand.type == Demand::Type::linear) {
                    updateUpper(upper, demand.additive);
                } else if (demand.type == Demand::Type::exponential) {
                    updateUpper(upper, demand.scale);
                } else {
                    throw std::logic_error("Unsupported demand type");
                }
            }
        // s_t
        } else if (event.type == InputData::Event::Type::arrival) {
            unsigned variableIndex = indexMap.timeToSIndex[relativeTime];
            updateLower(vlower[variableIndex], ZERO);
        // y_a^H
        }  else if (event.type == InputData::Event::Type::cutoff) {
            unsigned basedArcId = event.basedArc.value();
            unsigned variableIndex = indexMap.arcToHired[basedArcId];
            updateLower(vlower[variableIndex], ZERO);
        } else {
            throw std::logic_error("Unsupported event type");
        }
    }

    if (ignoreSpot) {
        for (const auto& event : input.events) {
            unsigned relativeTime = event.relativeTime;
            if (event.type == InputData::Event::Type::pricing) {
                for (unsigned index = 0; index < event.relatedItineraryIds.size(); ++index) {
                    unsigned idItinerary = event.relatedItineraryIds[index];
                    unsigned variableIndex =
                        indexMap.timeItineraryToDemandIndex[relativeTime][idItinerary];
                    vlower[variableIndex] = vupper[variableIndex] = 0;
                    if (event.demands[index].type == Demand::Type::exponential) {
                        const double EPS = 1e-40;
                        vupper[variableIndex] = EPS;
                    }
                }
            }
        }
    }
}

void IpoptBackend::initConstraintsLR(vector<double>* glowerPtr, vector<double>* gupperPtr) const {
    const auto& indexMap = indexManager->getConstData();
    const auto& input = config.inputManager->getConstData();

    assert(glowerPtr != nullptr && gupperPtr != nullptr);

    auto& glower = *glowerPtr;
    auto& gupper = *gupperPtr;

    const double INF = std::numeric_limits<double>::max();
    const double ZERO = 0.;
    const double ONE = 1.;

    glower.assign(indexMap.constraintCount, -INF);
    glower.shrink_to_fit();
    gupper.assign(indexMap.constraintCount, INF);
    gupper.shrink_to_fit();

    // arc capacity constraints
    for (const auto& arc : input.arcs) {
        if (arc.type == InputData::Arc::Type::travel) {
            unsigned constraintIndex = indexMap.arcCapacityConstraints[arc.id];
            updateUpper(gupper[constraintIndex - 1], ZERO);
        }
    }

    // positive in ports for cutoff and arrival
    for (const auto& event : input.events) {
        if (event.type == InputData::Event::Type::cutoff) {
            unsigned idBasedArc = event.basedArc.value();
            unsigned constraintIndex = indexMap.portPositiveCutoffArcConstraints[idBasedArc];
            updateLower(glower[constraintIndex - 1], ZERO);
        } else if (event.type == InputData::Event::Type::arrival) {
            unsigned idBasedArc = event.basedArc.value();
            unsigned constraintIndex = indexMap.portPositiveArrivalArcConstraints[idBasedArc];
            updateLower(glower[constraintIndex - 1], ZERO);
        }
    }

    // spotQN constraints
    for (const auto& itinerary : input.itineraries) {
        unsigned idItinerary = itinerary.id;
        unsigned constraintIndex = indexMap.spotQNConstraints[idItinerary];
        updateLower(glower[constraintIndex - 1], ZERO);
    }

    // final container constraints
    for (unsigned idPort = 0; idPort < input.ports.size(); ++idPort) {
        unsigned constraintIndex = indexMap.finalContainerConstraints[idPort];
        updateUpper(gupper[constraintIndex - 1], ZERO);
    }

    // group constraints
    for (unsigned idGroup = 0; idGroup < input.allotmentGroups.size(); ++idGroup) {
        unsigned constraintIndex = indexMap.groupConstraints[idGroup];
        updateLower(glower[constraintIndex - 1], ZERO);
        updateUpper(gupper[constraintIndex - 1], ONE);
    }

    // Q_a^i < u^i EN_a^i
    for (const auto& allotmentQConstraints : indexMap.allotmentItineraryQConstraints) {
        for (const auto& qConstraintIndex : allotmentQConstraints) {
            updateUpper(gupper[qConstraintIndex - 1], ZERO);
        }
    }
}


} // namespace backend
} // namespace sea

