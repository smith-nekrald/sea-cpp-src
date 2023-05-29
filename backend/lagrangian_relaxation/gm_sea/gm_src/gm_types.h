/**
 * @file gm_types.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines commonly used during gradient method implementation enumerations and types.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once


namespace gm {

/**
 * @brief Enumerates implemented Gradient Optimization methods.
 */
enum class GradientFamily {
    UFGM,   ///< Universal Fast Gradient Method
    GM,     ///< Simple Canonical Gradient Method.
    UPGM    ///< Universal Primal Gradient Method.
};

/**
 * @brief Enumerates prox-functions. 
 */
enum class GmProxType {
    L2SQUARED   ///< Prox-function in the form of squared L2 distance.
};

/**
 * @brief Enumerates regularizers.
 */
enum class GmRegularizerType {
    L2SQUARED   ///< Regularizer in the form of squared L2 distance.
};


} // namespace gm
