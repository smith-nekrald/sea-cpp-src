#pragma once

#include "types.h"
#include "input/input_data.h"

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <unordered_map>

namespace sea {

enum class ConvergenceCriterionType {
    VARIABLE_CHANGE,
    OBJECTIVE_CHANGE,
    BOTH
};

std::size_t getMemUsage();
void printTimeMemNow(std::ostream& ostream);

std::string makeUniqueFileName();
void printStory(const std::map<std::string, std::vector<double>>& story,
        std::string dirNamePrefix, std::string algoId);
void printStory(const std::map<std::string, std::vector<double>>& story,
        std::ofstream& out);


std::pair<ui32, ui32> getUsefulIndexCount(const std::vector<ui32>& target);
std::pair<ui32, ui32> getUsefulIndexCount(const std::vector<std::vector<ui32>>& target);

enum class AllotmentStrategyType {
    BENDERS,
    DET_CUT_PLANE,
    ZERO_IPOPT,
    ZERO_NULL,
    RANDOM_ONE_IPOPT,
    IPOPT
};

enum class SpotStrategyType {
    IPOPT,
    HYBRID,
    LR
};

enum class BackendType {
    BENDERS,
    IPOPT,
    DET_CUT_PLANE,
    LR
};


}   // namespace sea
