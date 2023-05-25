#pragma once

#include <chrono>
#include <jsoncpp/json/json.h>

#include "common.h"
#include "manager.h"
#include "backend/backend_general.h"

namespace json=Json;

using sea::backend::IpoptBackendConfig;
using sea::backend::LagrangianRelaxationBackendConfig;
using sea::backend::BendersAllotmentBackendConfig;
using sea::backend::DetCutPlaneBackendConfig;


namespace {

using namespace std::chrono;

class Timer {
public:
    Timer(): start(system_clock::now()) {}
    void restart() {
        start = system_clock::now();
    }
    seconds elapsed() const {
        return duration_cast<seconds>(system_clock::now() - start);
    }

private:
    system_clock::time_point start;
};

}


void runAll(json::Value& configRoot);

void fillBackendConfig(json::Value& configRoot, IpoptBackendConfig& config);
void fillBackendConfig(json::Value& configRoot, LagrangianRelaxationBackendConfig& config);
void fillBackendConfig(json::Value& configRoot, BendersAllotmentBackendConfig& config);
void fillBackendConfig(json::Value& configRoot, DetCutPlaneBackendConfig& config);

std::vector<std::string> getMarketDataFiles(std::string dataPath, std::size_t count_limit=10000);
sea::ConstInputManagerPtr getInput(std::string dataPath, bool needMemory);
sea::ConstLinksManagerPtr getLinks(const sea::ConstInputManagerPtr& inputManager,
        bool needMemory);
std::vector<sea::ConstMarketManagerPtr> getMarketVector(
        const std::vector<std::string>& pathVector, bool needMemory);

