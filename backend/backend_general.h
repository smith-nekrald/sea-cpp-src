#pragma once

#include "benders/benders_allotment_backend.h"
#include "ipopt/ipopt_backend.h"
#include "det_cut_plane/dcp_backend.h"
#include "lagrangian_relaxation/lagrangian_relaxation_backend.h"

#include <memory>

namespace sea {

struct BackendHolder {
    std::shared_ptr<backend::IpoptBackend> ipoptBackend = nullptr;
    std::shared_ptr<backend::LagrangianRelaxationBackend> lrBackend = nullptr;
    std::shared_ptr<backend::DetCutPlaneBackend> dcpBackend = nullptr;
    std::shared_ptr<backend::BendersAllotmentBackend> bendersBackend = nullptr;
};

inline void unsetBackends(BackendHolder& backends) {
    backends.ipoptBackend = nullptr;
    backends.lrBackend = nullptr;
    backends.dcpBackend = nullptr;
    backends.bendersBackend = nullptr;
}

template<typename Backend, typename Config>
inline void initBackend(std::shared_ptr<Backend>& backend_ptr, const Config& config) {
    if (backend_ptr == nullptr) {
        backend_ptr = std::make_shared<Backend>(config);
    }
}

template<typename Backend, typename Config>
inline void initBackend(std::shared_ptr<Backend>& backend_ptr,
        const Config& config, double utilizationRatio) {
    initBackend(backend_ptr, config);
    backend_ptr->setUtilizationRatio(utilizationRatio);
}

struct BackendConfigHolder {
    backend::IpoptBackendConfig ipoptConfig;
    backend::LagrangianRelaxationBackendConfig lrConfig;
    backend::DetCutPlaneBackendConfig dcpConfig;
    backend::BendersAllotmentBackendConfig bendersConfig;
};

} // namespace sea
