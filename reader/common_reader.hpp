/**
 * @file common_reader.hpp
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Template methods for reading input and market data.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cassert>


namespace sea {
namespace io {

namespace {

/**
 * @brief Splits string into tokens of a certain type.
 * 
 * @tparam Type The type of each token. 
 * @param[in] source The string to tokenize.
 * @param[out] tokens The resulting vector with tokens.
 */
template <typename Type = std::string>
inline void makeTokens(const std::string& source, std::vector<Type>& tokens) {
    std::istringstream inputStrStream(source);
    std::vector<Type> process{
        std::istream_iterator<Type>{inputStrStream}, std::istream_iterator<Type>{}};
    tokens.swap(process);
}

/**
 * @brief Reads line from stream and splits the line into tokens of a certain type.
 * 
 * @tparam Type The type of each token.
 * @param[out] stream The stream to read string from.
 * @param[out] tokens The resulting vector with tokens.
 * @return Updated stream for further applications.
 */
template <typename Type = std::string>
inline std::ifstream& makeTokens(std::ifstream& stream, std::vector<Type>& tokens) {
    std::string to_parse;
    while (to_parse.empty()) {
        std::getline(stream, to_parse);
    }
    makeTokens<Type>(to_parse, tokens);
    return stream;
}

} // namespace

// Re-definition for a function in header. To make further functions familiar with the signature. 
std::ifstream& validate(std::ifstream& stream, const std::string& header, unsigned& count);

/**
 * @brief Reads current data pattern from stream. For the implemented IO format, reads
 * header and number of entries through validate. Afterwards, reads count elements and sorts.
 * 
 * @tparam Type The type of entries to read.
 * @tparam Stream The type of stream to read from.
 * @param[out] stream The stream for reading information.
 * @param[in] header Header of the block.
 * @param[out] data Vector with read data of type Type.
 * @return The stream after reading.
 */
template <typename Type, typename Stream = std::ifstream>
inline std::ifstream& read(Stream& stream, const std::string& header, std::vector<Type>& data) {
    unsigned count = 0;
    validate(stream, header, count);

    for (unsigned ind = 0; ind < count; ++ind) {
        Type item;
        stream >> item;
        data.push_back(std::move(item));
    }

    std::sort(std::begin(data), std::end(data),
            [] (const Type& lhs, const Type& rhs) {return lhs.id < rhs.id;});
    return stream;
}

} // namespace io

} // namespace sea
