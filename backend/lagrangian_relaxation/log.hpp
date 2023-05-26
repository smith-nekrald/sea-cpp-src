#pragma once

#include "lr_cppad.h"
#include "../../logging/logging.h"

namespace sea {
namespace backend {

template<typename T>
void printArrayToBackendLog(const std::vector<T> array, BackendType type) {
    auto stream = logging::getBackendSubLogger(type) << log4cpp::Priority::DEBUG;
    stream << " Array: ";
    for (const auto& value : array) {
        stream << value << " ";
    }
}


} // namespace backend
} // namespace sea
