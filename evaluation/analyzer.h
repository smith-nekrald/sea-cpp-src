#pragma once

#include "evaluator.h"
#include "../manager.h"

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>

namespace sea {

class Analyzer {
public:
    void doAnalysis(
         const std::map<std::string, std::vector<Statistics>>& data,
         std::ostream& preciseOut,
         std::ostream& generalOut);
};

} // namespace sea
