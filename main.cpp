// Entry point to the program.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023
// (c) Smith School of Business, 2025

#include <fstream>
#include <cassert>
#include <set>
#include <filesystem>

#include <jsoncpp/json/json.h>

#include "start.h"

namespace json = Json;


/**
 * @brief Entry point. Expected application: ./<executable> <path-to-json-with-config>.
 *
 * @param argc The number of arguments in CLI.
 * @param argv The array with CLI arguments.
 * @return The program exit code.
 */
int main(int argc, char* argv[]) {
    const int N_ARGUMENTS = 2;
    const std::set<std::string> HELP_ARGUMENTS = {"--help", "-h"};
    bool helpCalled = false;
    for (unsigned idx = 1; idx < static_cast<unsigned>(argc); ++idx) {
        if (HELP_ARGUMENTS.find(std::string(argv[idx])) != HELP_ARGUMENTS.end()) {
            helpCalled = true;
            break;
        }
    }
    if (argc != N_ARGUMENTS || helpCalled) {
        std::cerr << "Usage: " << argv[0] << " <path-to-json-with-config>" << std::endl;
        return !helpCalled;
    }
    assert(argc == 2);
    std::string configPath = argv[1];
    std::ifstream configStream(configPath);
    json::Value configRoot;
    configStream >> configRoot;

    const char directorySeparator = '/';
    if (configPath[0] != directorySeparator) {
        const std::string relativePathPrefix = "./";
        configPath = relativePathPrefix + configPath;
    }

    std::string logPath = configPath.substr(0, configPath.find_last_of("/\\"));
    std::filesystem::current_path(logPath);
    start(configRoot);
}


