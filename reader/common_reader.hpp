#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>


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

} // namespace io

} // namespace sea
