#include "input_data.h"
#include <iostream>

namespace sea {

using std::cout;
using std::endl;
using EventType = InputData::Event::Type;

void printInputDataStats(const InputData& data) {
    cout << "=========================================" << endl;
    cout << "Statistics of InputData:" << endl;
    cout << "Number of ports: " << data.ports.size() << endl;
    cout << "Number of vessels: " << data.vessels.size() << endl;
    cout << "Number of nodes: " << data.nodes.size() << endl;
    cout << "Number of arcs: " << data.arcs.size() << endl;
    cout << "Number of allotment entires: " << data.allotmentEntries.size() << endl;
    cout << "Number of allotments: " << data.allotments.size() << endl;
    cout << "Number of groups: " << data.allotmentGroups.size() << endl;
    cout << "Number of itineraries: " << data.itineraries.size() << endl;
    cout << "Number of events: " << data.events.size() << endl;
    unsigned pricingCount = 0, cutoffCount = 0, arrivalCount = 0;
    for (const auto& event : data.events) {
        if (event.type == EventType::cutoff) {
            ++cutoffCount;
        } else if (event.type == EventType::pricing) {
            ++pricingCount;
        } else if (event.type == EventType::arrival) {
            ++arrivalCount;
        }
    }
    cout << "pricing events count: " << pricingCount << endl;
    cout << "cutoff events count: " << cutoffCount << endl;
    cout << "arrival events count: " << arrivalCount << endl;
    cout << "=========================================" << endl;
}

std::string eventTypeToName(const InputData::Event::Type& type) {
    if (type == InputData::Event::Type::pricing) {
        return "EventType::pricing";
    } else if (type == InputData::Event::Type::cutoff) {
        return "EventType::cutoff";
    } else if (type == InputData::Event::Type::arrival) {
        return "EventType::arrival";
    }
    throw std::logic_error("eventTypeToName: unsupported event type");
}

} // namespace sea
