/**
 * @file ireader.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Specifies Reader interface. Reader is an entity to read input.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <string>

namespace sea {
namespace io {

/**
 * @brief Interface for reading data.
 * 
 * @tparam DataType The type of data to read.
 */
template<typename DataType>
class IReader {
public:
    /**
     * @brief Reads the data from path.
     * 
     * @param[in] path The path to read data from. Either path to file or path to directory,
     * depending on the situation. 
     * @param[out] data The data to read.
     */
    virtual void Do(const std::string& path, DataType& data) const = 0;
    /**
     * @brief Virtual destructor. To ensure correct C++ polymorphism.
     */
    virtual ~IReader() {};
};

} // namespace io
} // namespace sea
