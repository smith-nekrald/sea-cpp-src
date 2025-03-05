/**
 * @file log.hpp
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Template logging methods.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "../../logging/logging.h"

namespace sea {
namespace backend {

/**
 * @brief Template method to print one-dimensional vector to backend log.
 *
 * @tparam Type The type of the element in the vector.
 * @param array The vector with elements to print.
 * @param type The backend type.
 */
template<typename Type>
void printArrayToBackendLog(const std::vector<Type> array, BackendType typeBackend) {
    auto stream = logging::getBackendSubLogger(typeBackend) << log4cpp::Priority::DEBUG;
    stream << " Array: ";
    for (const auto& value : array) {
        stream << value << " ";
    }
}

} // namespace backend
} // namespace sea
