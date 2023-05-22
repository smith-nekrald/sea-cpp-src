// Implements Analyzer methods and related API.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

#include "analyzer.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>

namespace sea {

using std::map;
using std::string;
using std::vector;
using std::endl;
using std::ostream;
using std::size_t;


// Phi(-∞, x) aka N(x)
double normalCDF(double xPoint) {
    return std::erfc(-xPoint / std::sqrt(2.)) / 2.;
}

void writeSingleInfo(
        const string& algoName, const vector<Statistics>& data, ostream& preciseOut) {
    preciseOut << "Algorithm: " << algoName << endl << endl;
    double meanProfit = 0, stdProfit = 0;
    double meanPercentage = 0, stdPercentage = 0;;

    for (size_t id = 0; id < data.size(); ++id) {
        preciseOut << "Launch: " << id << endl;
        printEvaluatorStatistics(data[id], preciseOut);
        preciseOut << endl;
        meanProfit += data[id].fullProfit;
        const double EPS_NONZERO = 1e-20;
        meanPercentage += data[id].fullProfit
            / (data[id].estimation.spotMarket + data[id].estimation.allotment + EPS_NONZERO);
    }
    meanProfit /= data.size();
    meanPercentage /= data.size();

    for (size_t id = 0; id < data.size(); ++id) {
        stdProfit += (data[id].fullProfit - meanProfit)
                    * (data[id].fullProfit - meanProfit);
        const double EPS_NONZERO = 1e-20;
        double percentage =  data[id].fullProfit
            / (data[id].estimation.spotMarket + data[id].estimation.allotment + EPS_NONZERO);
        stdPercentage += percentage * percentage;
    }
    if (data.size() <= 1)  {
        stdPercentage = stdProfit = 0;
    } else {
        stdPercentage /= (data.size() - 1);
        stdProfit /= (data.size() - 1);
        stdPercentage = sqrt(stdPercentage);
        stdProfit = sqrt(stdProfit);
    }
    preciseOut << "GeneralStats: " << endl;
    preciseOut << "meanProfit = " << meanProfit << " stdProfit = " << stdProfit << endl;
    preciseOut << "meanPercentage = " << meanPercentage
        << " stdPercentage = " << stdPercentage << endl;
    preciseOut << endl;
}

void writeComparation(
        const string& lhsAlgo,
	const string& rhsAlgo,
        const vector<Statistics>& lhsData,
	const vector<Statistics>& rhsData,
        ostream& generalOut) {

    assert(lhsData.size() == rhsData.size());
    vector<double> profitDiff;

    const double INF = std::numeric_limits<double>::max();
    double maxAbsoluteRelative = -INF;
    double diffMean = 0;

    bool alwaysPositive = true;
    bool alwaysNegative = true;
    size_t dataSize = lhsData.size();
    for (size_t id = 0; id < lhsData.size(); ++id) {
        profitDiff.push_back(lhsData[id].fullProfit - rhsData[id].fullProfit);
        maxAbsoluteRelative = std::max(maxAbsoluteRelative, std::fabs(profitDiff.back())
                / (lhsData[id].estimation.allotment + lhsData[id].estimation.spotMarket));
        alwaysNegative = alwaysNegative && (profitDiff.back() <= 0);
        alwaysPositive = alwaysPositive && (profitDiff.back() >= 0);
        diffMean += profitDiff.back();
    }
    diffMean /= profitDiff.size();
    double diffStd = 0;
    for (size_t id = 0; id < lhsData.size(); ++id) {
        diffStd += (profitDiff[id] - diffMean) * (profitDiff[id] - diffMean);
    }

    if (profitDiff.size() <= 1) {
        diffStd = 0;
    } else {
        diffStd /= (profitDiff.size() - 1);
        diffStd = sqrt(diffStd);
    }

    generalOut << lhsAlgo << " vs " << rhsAlgo << endl;
    generalOut << "alwaysNegative (lhs < rhs): " << std::boolalpha << alwaysNegative << endl;
    generalOut << "alwaysPositive (lhs > rhs): " << std::boolalpha << alwaysPositive << endl;
    generalOut << "meanDiff = " << diffMean << endl;
    generalOut << "stdDiff = " << diffStd << endl;

    const double REL_EPS = 1e-2;
    generalOut << "MAX_ABSOLUTE_RELATIVE: " << maxAbsoluteRelative  <<
        " parity_by_relative_error: " << std::boolalpha <<
        (maxAbsoluteRelative < REL_EPS) << std::endl;

    const double EPS = 1e-1;
    if (diffStd < EPS) {
        diffStd = EPS;
    }
    double testRatio = diffMean / diffStd * std::sqrt(dataSize);
    double confidence = normalCDF(std::abs(testRatio));

    if (testRatio > EPS) {
        generalOut << "CONFIDENCE: " << lhsAlgo << " wins " << rhsAlgo;
    } else if (-testRatio > EPS) {
        generalOut << "CONFIDENCE: " << rhsAlgo << " wins " << lhsAlgo;
    } else {
        generalOut << "CONFIDENCE: " << lhsAlgo << " equals " << rhsAlgo;
    }
    generalOut << " with confidence " << confidence * 100 << endl;

    const double HYPOTHESIS_BOUND = 1.645;
    double testFirstBest = diffMean - HYPOTHESIS_BOUND * diffStd / std::sqrt(dataSize);
    double testSecondBest = diffMean + HYPOTHESIS_BOUND * diffStd / std::sqrt(dataSize);
    generalOut << "testFirstBest = " << testFirstBest << endl;
    generalOut << "testSecondBest = " << testSecondBest << endl;
    if (testFirstBest > 0) {
        generalOut << lhsAlgo <<  " > (is_better_than) " << rhsAlgo << endl;
    } else if (testSecondBest < 0)  {
        generalOut << lhsAlgo << " < (is_worse_than) " << rhsAlgo << endl;
    } else {
        generalOut << lhsAlgo << " ? (uncertain_testing) " << rhsAlgo << endl;
    }
}

void Analyzer::doAnalysis(
        const map<string, vector<Statistics>>& data, ostream& preciseOut, ostream& generalOut) {
    vector<string> algoNames;
    for (const auto& value_pair : data) {
        algoNames.push_back(value_pair.first);
    }
    for (size_t firstId = 0; firstId < algoNames.size(); ++firstId) {
        const auto& firstName = algoNames[firstId];
        // Summarize each algorithm.
        writeSingleInfo(firstName, data.at(firstName), preciseOut);
        // Compare each algorithms pair.
        for (size_t secondId = firstId + 1; secondId < algoNames.size(); ++secondId) {
            const auto& secondName = algoNames[secondId];
            writeComparation(
                    firstName, secondName, data.at(firstName), data.at(secondName), generalOut);
        }
    }
}

} // namespace sea
