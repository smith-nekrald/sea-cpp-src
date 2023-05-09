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

void IpoptBackend::initBoundsLR(vector<double>* vlowerPtr, vector<double>* vupperPtr) {
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
    for (ui32 idItinerary = 0; idItinerary < input.itineraries.size(); ++idItinerary) {
        ui32 indexQr = indexMap.idItineraryToQIndex[idItinerary];
        updateLower(vlower[indexQr], double(0.));
        ui32 indexZr = indexMap.idItineraryToZIndex[idItinerary];
        updateLower(vlower[indexZr], double(0.));
        for (ui32 idAllotment : links.allotmentsWithItinerary[idItinerary]) {
            ui32 indexQir = indexMap.allotmentItineraryToQIndex[idAllotment][idItinerary];
            updateLower(vlower[indexQir], double(0.));
            ui32 indexEntry = links.allotmentItineraryToEntry.at(idAllotment).at(idItinerary);
            const auto& entry = input.allotmentEntries[indexEntry];
            double expectedShow = entry.productAmount * entry.showRate.estimatedProba;
            updateUpper(vupper[indexQir], expectedShow);
        }
    }

    // u_i
    for (ui32 idAllotment = 0; idAllotment < input.allotments.size(); ++idAllotment) {
        ui32 indexUi = indexMap.allotmentToUIndex[idAllotment];
        updateLower(vlower[indexUi], double(0.0));
        updateUpper(vupper[indexUi], double(1.0));
    }

    // d_t, s_t, y_a^H
    for (const auto& event : input.events) {
        ui32 relativeTime = event.relativeTime;
        // d_t
        if (event.type == InputData::Event::Type::pricing) {
            for (ui32 index = 0; index < event.relatedItineraryIds.size(); ++index) {
                ui32 idItinerary = event.relatedItineraryIds[index];
                const auto& demand = event.demands[index];
                ui32 variableIndex = indexMap.timeItineraryToDemandIndex[relativeTime][idItinerary];
                double& lower = vlower[variableIndex];
                double& upper = vupper[variableIndex];

                updateLower(lower, double(0.));
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
            ui32 variableIndex = indexMap.timeToSIndex[relativeTime];
            updateLower(vlower[variableIndex], double(0.));
        // y_a^H
        }  else if (event.type == InputData::Event::Type::cutoff) {
            ui32 basedArcId = event.basedArc.value();
            ui32 variableIndex = indexMap.arcToHired[basedArcId];
            updateLower(vlower[variableIndex], double(0.));
        } else {
            throw std::logic_error("Unsupported event type");
        }
    }

    if (ignoreSpot) {
        for (const auto& event : input.events) {
            ui32 relativeTime = event.relativeTime;
            if (event.type == InputData::Event::Type::pricing) {
                for (ui32 index = 0; index < event.relatedItineraryIds.size(); ++index) {
                    ui32 idItinerary = event.relatedItineraryIds[index];
                    ui32 variableIndex =
                        indexMap.timeItineraryToDemandIndex[relativeTime][idItinerary];
                    vlower[variableIndex] = vupper[variableIndex] = 0;
                    if (event.demands[index].type == Demand::Type::exponential) {
                        vupper[variableIndex] = 1e-40;
                    }
                }
            }
        }
    }
}

void IpoptBackend::initConstraintsLR(vector<double>* glowerPtr, vector<double>* gupperPtr) {
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
            ui32 constraintIndex = indexMap.arcCapacityConstraints[arc.id];
            updateUpper(gupper[constraintIndex - 1], double(0.));
        }
    }

    // positive in ports for cutoff and arrival
    for (const auto& event : input.events) {
        if (event.type == InputData::Event::Type::cutoff) {
            ui32 idBasedArc = event.basedArc.value();
            ui32 constraintIndex = indexMap.portPositiveCutoffArcConstraints[idBasedArc];
            updateLower(glower[constraintIndex - 1], double(0.));
        } else if (event.type == InputData::Event::Type::arrival) {
            ui32 idBasedArc = event.basedArc.value();
            ui32 constraintIndex = indexMap.portPositiveArrivalArcConstraints[idBasedArc];
            updateLower(glower[constraintIndex - 1], double(0.));
        }
    }

    // spotQN constraints
    for (const auto& itinerary : input.itineraries) {
        ui32 idItinerary = itinerary.id;
        ui32 constraintIndex = indexMap.spotQNConstraints[idItinerary];
        updateLower(glower[constraintIndex - 1], double(0.));
    }

    // final container constraints
    for (ui32 idPort = 0; idPort < input.ports.size(); ++idPort) {
        ui32 constraintIndex = indexMap.finalContainerConstraints[idPort];
        updateUpper(gupper[constraintIndex - 1], double(0.));
    }

    // group constraints
    for (ui32 idGroup = 0; idGroup < input.allotmentGroups.size(); ++idGroup) {
        ui32 constraintIndex = indexMap.groupConstraints[idGroup];
        updateLower(glower[constraintIndex - 1], double(0.));
        updateUpper(gupper[constraintIndex - 1], double(1.));
    }
    if (config.useEnhancedVersion) {
        for (const auto& allotmentQConstraints : indexMap.allotmentItineraryQConstraints) {
            for (const auto& qConstraintIndex : allotmentQConstraints) {
                updateUpper(gupper[qConstraintIndex - 1], double(0.));
            }
        }

    }
}


} // namespace backend
} // namespace sea

