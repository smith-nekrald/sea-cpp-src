// Implementation for methods declared in common.h

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

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

using std::vector;

std::size_t getMemUsage() {
    std::ifstream statStream("/proc/self/stat", std::ios_base::in);
    // Variables to read before the desired one.
    std::string pid, comm, state, ppid, pgrp, session, ttyNr, tpgid, flags,
                minflt, cminflt, majflt, cmajflt, utime, stime, cutime, cstime,
                priority, nice, O, itervalue, starttime;

    // The desired target entry.
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
    auto inTimeT = std::chrono::system_clock::to_time_t(now);
    ostream << "Time: " <<  std::put_time(std::localtime(&inTimeT), "%y-%m-%d %X") << std::endl;
    ostream << "Memory usage: " << getMemUsage() / (1024. * 1024) << " MB" << std::endl;
}

std::pair<unsigned, unsigned> getUsefulIndexCount(const vector<unsigned>& target) {
    const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();
    std::pair<unsigned, unsigned> ans = {0, 0};
    for (unsigned entry : target) {
        if (entry != MAX_INDEX) {
            ++ans.first;
        }
        ++ans.second;
    }
    return ans;
}

std::pair<unsigned, unsigned> getUsefulIndexCount(const vector<vector<unsigned>>& target) {
    const unsigned MAX_INDEX = std::numeric_limits<unsigned>::max();
    std::pair<unsigned, unsigned> ans = {0, 0};
    for (auto& entry : target) {
        for (unsigned item : entry) {
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
    std::ofstream out(prefix + "/story_" + std::to_string(call_time)
            +  "_" +  makeUniqueFileName());
    out << "Algo ID: " << algoId << std::endl;
    out << "Call Time: " << call_time << std::endl;
    printStory(story, out);
    call_time += 1;
}

void printStory(const std::map<std::string, std::vector<double>>& story, std::ofstream& out) {
    for (const auto& item : story) {
        out << item.first << " : " << std::endl;
        for (const auto& value : item.second) {
            out << value << " ";
        }
        out << std::endl;
    }
}

std::string makeUniqueFileName() {
    uuid_t generateOut;
    uuid_generate_random(generateOut);
    char textOut[100];
    uuid_unparse(generateOut, textOut);
    return std::string(textOut);
}

} // namespace sea
