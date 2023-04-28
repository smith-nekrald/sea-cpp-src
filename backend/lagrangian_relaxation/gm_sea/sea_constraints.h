#pragma once

#include "gm_src/gm_interfaces.h"
#include "sea_gm_apply.h"

namespace sea {
namespace backend {

class SeaQDescriptor : public gm::IQDescriptor {

public:
    SeaQDescriptor(
            ManagerStorage storage,
            const State& state,
            const LagrangianRelaxationBackendConfig& config)
        : storage(storage)
        , state(state)
        , config(config) {
    }
    virtual std::vector<CppAD::AD<double>> makeConstraints(
            const std::vector<CppAD::AD<double>>& variables) const override;
    virtual void initConstraintsLR(
            std::vector<double>* glower, std::vector<double>* gupper) const override;
    virtual void initBoundsLR(
            std::vector<double>* vlower, std::vector<double>* vupper) const override;
    virtual ~SeaQDescriptor() {};

private:
    const ManagerStorage storage;
    const State state;
    const LagrangianRelaxationBackendConfig config;
};

} // namespace backend
} // namespace sea
