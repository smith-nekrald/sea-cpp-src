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

template <typename Type = std::string>
inline void makeTokens(const std::string& source, std::vector<Type>& tokens) {
    std::istringstream inputStrStream(source);
    std::vector<Type> process{
        std::istream_iterator<Type>{inputStrStream}, std::istream_iterator<Type>{}};
    tokens.swap(process);
}

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

std::ifstream& validate(std::ifstream& stream, const std::string& header, unsigned& count);

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
