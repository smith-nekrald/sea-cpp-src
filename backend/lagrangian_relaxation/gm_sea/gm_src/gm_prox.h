#pragma once

#include "gm_common.h"
#include "gm_interfaces.h"


#include <cppad/ipopt/solve.hpp>
#include <vector>
#include <memory>

namespace gm {

template<typename Type>
class ProxL2Squared : public IOrderOneOracle<Type> {
public:
    ProxL2Squared() {}
    virtual Type getValue(const std::vector<Type>& point) const {
        return l2normSquared<Type>(point) / 2;
    }
    virtual std::vector<Type> getSubgradient(const std::vector<Type>& point) const {
        std::vector<Type> result = point;
        return result;
    }
    virtual ~ProxL2Squared() {};
};


} // namespace gm

