#pragma once

#include <string>

namespace sea {
namespace io {

template<typename DataType>
class IReader {
public:
    virtual void Do(const std::string& directory, DataType& data) const = 0;
    virtual ~IReader() {};
};

} // namespace io
} // namespace sea
