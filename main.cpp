#include <iostream>
#include <fstream>
#include <cassert>
#include <filesystem>

#include <jsoncpp/json/json.h>

#include "start.h"

namespace json = Json;


/**
 * @brief Entry point. Expected application: ./<executable> <path-to-json-with-config>.
 *
 * @param argc The number of arguments in CLI.
 * @param argv The array with CLI arguments
 * @return The program exit code.
 */
int main(int argc, char* argv[]) {
    assert(argc == 2);
    std::string configPath = argv[1];
    std::ifstream configStream(configPath);
    json::Value configRoot;
    configStream >> configRoot;

    if (configPath[0] != '/') {
        configPath = std::string("./") + configPath;
    }

    std::string logPath = configPath.substr(0, configPath.find_last_of("/\\"));
    std::filesystem::current_path(logPath);
    runAll(configRoot);
}


