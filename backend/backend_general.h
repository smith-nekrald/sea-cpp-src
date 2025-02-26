/**
 * @file backend_general.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Common structures and methods for backends. Holders for all backends and configurations
 * to simplify function declaration.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "benders/benders_allotment_backend.h"
#include "ipopt/ipopt_backend.h"
#include "det_cut_plane/dcp_backend.h"
#include "lagrangian_relaxation/lagrangian_relaxation_backend.h"
#include "baseline/backend.h"

#include <memory>

namespace sea {

/**
 * @brief Holder with fields for all backends. Typically, only those which are required
 * are filled.
 */
struct BackendHolder {
    /// @brief Pointer to IpoptBackend.
    std::shared_ptr<backend::IpoptBackend> ipoptBackend = nullptr;
    /// @brief Pointer to LR Backend.
    std::shared_ptr<backend::LagrangianRelaxationBackend> lrBackend = nullptr;
    /// @brief Pointer to Deterministic Cutting Plane allotment backend.
    std::shared_ptr<backend::DetCutPlaneBackend> dcpBackend = nullptr;
    /// @brief Pointer to Benders allotment backend.
    std::shared_ptr<backend::BendersAllotmentBackend> bendersBackend = nullptr;
};

/**
 * @brief Unsets backends. Sets pointers in a Backend Holder to nullptr.
 *
 * @param[out] backends The Holder to process.
 */
inline void unsetBackends(BackendHolder& backends) {
    backends.ipoptBackend = nullptr;
    backends.lrBackend = nullptr;
    backends.dcpBackend = nullptr;
    backends.bendersBackend = nullptr;
}

/**
 * @brief Generic initialization for backend.
 *
 * @tparam Backend The type of backend.
 * @tparam Config The type of backend configuration.
 * @param backend_ptr Pointer to a backend object.
 * @param config  Backend object configuration.
 */
template<typename Backend, typename Config>
inline void initBackend(std::shared_ptr<Backend>& backend_ptr, const Config& config) {
    if (backend_ptr == nullptr) {
        backend_ptr = std::make_shared<Backend>(config);
    }
}

/**
 * @brief Generic backend initialization with setting utilization ratio.
 *
 * @tparam Backend The Type of Backend to initialize.
 * @tparam Config The Type of Backend configuration.
 * @param backend_ptr Pointer to backend.
 * @param config Backend configuration.
 * @param utilizationRatio Maximial capacity utilization ratio.
 */
template<typename Backend, typename Config>
inline void initBackend(std::shared_ptr<Backend>& backend_ptr,
        const Config& config, double utilizationRatio) {
    initBackend(backend_ptr, config);
    backend_ptr->setUtilizationRatio(utilizationRatio);
}

/**
 * @brief Holder with Backend Configurations. Typically, only relevant configs get initialized.
 */
struct BackendConfigHolder {
    /// @brief Ipopt Backend Configuration.
    backend::IpoptBackendConfig ipoptConfig;
    /// @brief Configuration for LR Backend.
    backend::LagrangianRelaxationBackendConfig lrConfig;
    /// @brief Configuration for DCP Backend.
    backend::DetCutPlaneBackendConfig dcpConfig;
    /// @brief Configuration for Benders Backend.
    backend::BendersAllotmentBackendConfig bendersConfig;
};

} // namespace sea
