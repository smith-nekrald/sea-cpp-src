/**
 * @file analyzer.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Defines Analyzer entity, which is responsible for algorithm summarizing and comparison
 * after the evaluation is ready. Also implements related API.
 * @copyright (c) Smith School of Business, 2023
 */
#pragma once

#include "evaluator.h"
#include "../manager.h"

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>

namespace sea {

/**
 * @brief Analysis after the evaluation is ready.
 */
class Analyzer {
public:
    /**
     * @brief Method to start evaluation analysis.
     *
     * @param data The collected evaluation data. Each pair holds algorithm name and a vector
     * with evaluation statistics, one statistics per one market data.
     * @param preciseOut The stream to write single-algorithm summary.
     * @param generalOut The stream to write comparisons for all possible pairs of algorithms.
     */
    void doAnalysis(
         const std::map<std::string, std::vector<Statistics>>& data,
         std::ostream& preciseOut,
         std::ostream& generalOut);
};

/**
 * @brief Computes CDF of Normal Distribution.
 *
 * @param xPoint Point to compute the CDF at.
 * @return The CDF value at point, N(xPoint).
 */
double normalCDF(double xPoint);
/**
 * @brief Method to write evaluation summary for a single algorithm.
 *
 * @param[in] algoName The algorithm name.
 * @param[in] data The vector with evaluation statistics, one evaluation for one market data.
 * @param[out] preciseOut The stream to write the summary.
 */
void writeSingleInfo(
        const std::string& algoName,
        const std::vector<Statistics>& data,
        std::ostream& preciseOut);
/**
 * @brief Method to compare pairs of algorithms and write summary into a corresponding stream.
 *
 * @param[in] lhsAlgo The name of first algorithm.
 * @param[in] rhsAlgo The name of second algorithm.
 * @param[in] lhsData Evaluation statistics for the first algorithm.
 * @param[in] rhsData Evaluation statistics for the second algorithm.
 * @param[out] generalOut The stream to write the comparison summary.
 */
void writeComparation(
        const std::string& lhsAlgo,
        const std::string& rhsAlgo,
        const std::vector<Statistics>& lhsData,
        const std::vector<Statistics>& rhsData,
        std::ostream& generalOut);
} // namespace sea
