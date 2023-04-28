#pragma once

#include "protocol.h"

namespace sea {

using std::endl;
using std::size_t;

template<typename T>
void writeToStream(
        T& out,
        const Decision& decision) {
    out << "================" << std::endl;
    out << "Decision:" << std::endl;
    out << "timeNow " << decision.time << std::endl;
    out << "off-hired_in_ports_(by_ports):" << std::endl;
    out << decision.offHiredInPortS.size() << endl;
    for (std::size_t t = 0; t < decision.offHiredInPortS.size(); ++t) {
        out << "time " << t << " size " << decision.offHiredInPortS[t].size() << std::endl;
        for (auto count : decision.offHiredInPortS[t]) {
            out << count << " ";
        }
        out << std::endl;
    }
    out << "prices_[time][print_pairs:_<itinerary,price>]:" << std::endl;
    out << "time_size " << decision.prices.size() << std::endl;
    for (std::size_t t = 0; t < decision.prices.size(); ++t) {
        out << "time " << t << std::endl;
        out << "count " << decision.prices[t].size() << endl;
        for (const auto & p : decision.prices[t]) {
            out << p.first << " " << p.second << " " << endl;
        }
    }
    out << "empty_containers_(Z)_(by_itineraries):" << std::endl;
    out << "size " << decision.emptyContainersZ.size() << endl;
    for (auto z : decision.emptyContainersZ) {
        out << z << " ";
    }
    out << std::endl;

    out << "size " << decision.nonEmptyContainersQ.size() << endl;
    out << "non-empty_containers_(Q)_(by itineraries):" << std::endl;
    for (auto q : decision.nonEmptyContainersQ) {
        out << q << " ";
    }
    out << std::endl;

    out << "allotment_non-empty_containers_(Q)[contractId][<itinerary,q>:" << std::endl;
    out << "size " << decision.allotmentContainersQ.size() << endl;
    for (std::size_t contractId = 0; contractId < decision.allotmentContainersQ.size(); ++contractId) {
        out << "contract-id " << contractId << std::endl;
        out << "size " << decision.allotmentContainersQ[contractId].size() << endl;
        for (const auto & p : decision.allotmentContainersQ[contractId]) {
            out << p.first << " " << p.second << " " << endl;
        }
    }
    out << "hired_containers_(Y)_(by_arcs):" << std::endl;
    for (auto h : decision.hiredY) {
        out << h << " ";
    }
    out << std::endl;

    out << "accepted_allotments:" << std::endl;
    out << "size " << decision.allotmentAccepted.size() << endl;
    for (std::size_t i  = 0; i < decision.allotmentAccepted.size(); ++i) {
        if (decision.allotmentAccepted[i])
            out << i << " ";
    }
    out << std::endl;
    out << "================" << std::endl;

}

template<typename T>
void writeToStream(
        T& writer,
        const Action& filledAction) {
    writer << "================" << std::endl;
    writer << "Action: " << std::endl;
    writer << "Time = " << filledAction.time << std::endl;
    writer << "spotMarketDemand_(N)_(by_itineraries):" << std::endl;
    writer << filledAction.spotMarketDemandN.size() << std::endl;
    for (const auto& value : filledAction.spotMarketDemandN) {
        writer << value  << " ";
    }
    writer << std::endl;
    writer << "bookings_(B)_(by_time_and_itineraries): " << std::endl;
    writer << filledAction.bookingsB.size() << endl;
    for (size_t time = 0; time < filledAction.bookingsB.size(); ++time) {
        auto& bookings = filledAction.bookingsB[time];
        writer << "time: " <<  time << "itineraries_count: " <<  filledAction.bookingsB[time].size() << endl;
        writer << "bookings: (indexed by itineraries) " << endl;
        for (const auto& value : bookings) {
            writer << value << " ";
        }
        writer << endl;
    }
    writer << "allotment_demand_(N)_(by_allotments):" << endl;
    writer << filledAction.allotmentDemandN.size() << endl;
    for (std::size_t idx = 0; idx < filledAction.allotmentDemandN.size(); ++idx) {
        writer << "contract_id " << idx << std::endl;
        for (const auto & pr : filledAction.allotmentDemandN[idx]) {
            writer << pr.first << " " << pr.second << " " << endl;
        }
    }
    writer << "================" << std::endl;
}



} // namespace sea
