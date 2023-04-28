#pragma once

#include "evaluator.h"
#include "../manager.h"

#include <vector>
#include <string>
#include <map>

namespace sea {

struct LauncherConfig {
    std::vector<algo::IAlgorithmPtr> algorithms;
    std::vector<std::string> marketDataSetPaths;
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
    std::experimental::optional<bool> needMemory;
    std::experimental::optional<bool> writeStory;
};

class Launcher {
public:
    explicit Launcher(const LauncherConfig& aConfig);
    std::map<std::string, std::vector<Statistics>> doLaunches();
private:
    LauncherConfig config;
};

} // namespace sea
