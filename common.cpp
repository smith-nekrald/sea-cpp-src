#include "common.h"

#include <uuid/uuid.h>

#include <chrono>
#include <ctime>
#include <exception>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <limits>

namespace sea {

const ui32 MAX_INDEX = std::numeric_limits<ui32>::max();
const double INF = std::numeric_limits<double>::max();

using std::vector;

std::size_t getMemUsage() {
    std::ifstream statStream("/proc/self/stat", std::ios_base::in);

    // dummy vars
    std::string pid, comm, state, ppid, pgrp, session, ttyNr, tpgid, flags,
                minflt, cminflt, majflt, cmajflt, utime, stime, cutime, cstime,
                priority, nice, O, itervalue, starttime;

    // what we want
    std::size_t vmSize;

    statStream >> pid >> comm >> state >> ppid >> pgrp >> session >> ttyNr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itervalue >> starttime >> vmSize;

    statStream.close();

    return vmSize;
}

void printTimeMemNow(std::ostream& ostream) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    ostream << "Time: " <<  std::put_time(std::localtime(&in_time_t), "%y-%m-%d %X") << std::endl;
    ostream << "Memory usage: " << getMemUsage() / (1024. * 1024) << " MB" << std::endl;
}

std::pair<ui32, ui32> getUsefulIndexCount(const vector<ui32>& target) {
    std::pair<ui32, ui32> ans = {0, 0};
    for (ui32 entry : target) {
        if (entry != MAX_INDEX) {
            ++ans.first;
        }
        ++ans.second;
    }
    return ans;
}

std::pair<ui32, ui32> getUsefulIndexCount(const vector<vector<ui32>>& target) {
    std::pair<ui32, ui32> ans = {0, 0};
    for (auto& entry : target) {
        for (ui32 item : entry) {
            if (item != MAX_INDEX) {
                ++ans.first;
            }
            ++ans.second;
        }
    }
    return ans;
}

void printStory(const std::map<std::string, std::vector<double>>& story,
        std::string prefix, std::string algoId) {
    static size_t call_time = 0;
    std::ofstream out(prefix + "/story_" + std::to_string(call_time) +  "_" +  makeUniqueFileName());
    out << "Algo ID: " << algoId << std::endl;
    out << "Call Time: " << call_time << std::endl;
    printStory(story, out);
    call_time += 1;
}

void printStory(const std::map<std::string, std::vector<double>>& story,
        std::ofstream& out) {
    for (const auto& item : story) {
        out << item.first << " : " << std::endl;
        for (const auto& value : item.second) {
            out << value << " ";
        }
        out << std::endl;
    }
}

std::string makeUniqueFileName() {
    uuid_t tempOut;
    uuid_generate_random(tempOut);
    char textOut[100];
    uuid_unparse(tempOut, textOut);
    return std::string(textOut);
}

} // namespace sea
