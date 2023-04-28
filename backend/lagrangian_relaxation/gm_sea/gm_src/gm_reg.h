#pragma once

#include "gm_interfaces.h"
#include "gm_common.h"

#include <vector>
#include <memory>

namespace gm {

template<typename Type>
class L2SquaredReg : public IOrderOneOracle<Type> {
public:
    L2SquaredReg(double coeff, std::vector<double> aMean=std::vector<double>())
        : coeff(coeff)
        , mean(aMean) {
    };
    virtual Type getValue(const std::vector<Type>& point) const {
        return coeff * l2normSquared<Type>(point);
    };
    virtual std::vector<Type> getSubgradient(const std::vector<Type>& point) const {
        auto adPoint = makeADRoutine(point);
        auto adCopy = adPoint;
        if (adPoint.size() == mean.size()) {
            for (size_t idx = 0; idx < adPoint.size(); ++idx) {
                adCopy[idx] -= mean[idx];
            }
        }
        CppAD::AD<Type> entry = coeff * l2normSquared<CppAD::AD<Type>>(adCopy);
        std::vector<CppAD::AD<Type>> objective = {entry};
        return subgradientRoutine<Type>(point, adPoint, objective);
    };
    virtual ~L2SquaredReg() {};
private:
    double coeff;
    std::vector<double> mean;
};

} // namespace gm
