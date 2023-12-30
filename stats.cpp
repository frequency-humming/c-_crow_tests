#include "stats.h"
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>

std::vector<Stats> getStats() {
    std::vector<Stats> stats;
    stats.emplace_back("cpuInfo", execCommand("sysctl -n machdep.cpu.brand_string", std::bitset<3>{0b000}));
    stats.emplace_back("osInfo", execCommand("sw_vers -productName", std::bitset<3>{0b000}));
    stats.emplace_back("osVersion", execCommand("sw_vers -productVersion", std::bitset<3>{0b000}));
    stats.emplace_back("hostname", execCommand("hostname", std::bitset<3>{0b000}));
    stats.emplace_back("cpuCount", execCommand("sysctl -n hw.ncpu", std::bitset<3>{0b000}));
    stats.emplace_back("cpuCores", execCommand("sysctl -n hw.physicalcpu", std::bitset<3>{0b000}));
    stats.emplace_back("cpuThreads", execCommand("sysctl -n hw.logicalcpu", std::bitset<3>{0b000}));
    stats.emplace_back("disk", execCommand("df -h", std::bitset<3>{0b010}));
    stats.emplace_back("uptime", execCommand("uptime", std::bitset<3>{0b000}));
    stats.emplace_back("cpuUsage", execCommand("top -l 1 | grep CPU", std::bitset<3>{0b100}));
    stats.emplace_back("memoryUsage", execCommand("top -l 1 | grep PhysMem", std::bitset<3>{0b000}));
    stats.emplace_back("diskUsage", execCommand("top -l 1 | grep Disk", std::bitset<3>{0b100}));
    stats.emplace_back("networkUsage", execCommand("top -l 1 | grep Network", std::bitset<3>{0b000}));
    return stats;
}

void parseResponse(std::vector<Stats>& stats) {
    std::string memory = execCommand("sysctl hw.memsize", std::bitset<3>{0b000});
    std::regex non_digit("[^0-9]");
    std::string only_digits = std::regex_replace(memory, non_digit, "");
    long number = std::stoll(only_digits) / 1024 / 1024 / 1024;
    std::string ram = std::to_string(number) + "GB";
    stats.emplace_back("memory", ram);
}

std::future<std::string> runTracerouteAsync(const std::string& endpoint) {
    return std::async(std::launch::async, [endpoint]() {
        try {
            std::string command = "traceroute -m 10 " + endpoint;
            return execCommand(command.c_str(), std::bitset<3>{0b001});
        } catch (const std::runtime_error& e) {
            std::cerr << "Traceroute Error: " << e.what() << std::endl;
            return "Traceroute failed: " + std::string(e.what());
        }
    });
}

std::string execCommand(const char* cmd, std::bitset<3> v) {
    std::array<char, 200> buffer;
    std::string result;
    std::string mountPoint;
    bool var = false;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    std::string tableHtml = "<table class='table-auto w-full'><tbody>";
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    try {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            if (v.test(0)) {
                result += buffer.data();
                result += "<br>";
            } else if (v.test(1)) {
                std::istringstream iss(buffer.data());
                std::vector<std::string> parts(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
                tableHtml += "<tr>";
                for (const auto& part : parts) {
                    if (part == "Mounted" || part == "map") {
                        mountPoint = part;
                        var = true;
                    } else if (var) {
                        mountPoint += " " + part;
                        tableHtml += "<td>" + mountPoint + "</td>";
                        var = false;
                    } else {
                        tableHtml += "<td>" + part + "</td>";
                    }
                }
                tableHtml += "</tr>";
            } else if (v.test(2)) {
                result = buffer.data();
                return result;
            } else {
                result += buffer.data();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    if (v.test(1)) {
        tableHtml += "</tbody></table>";
        result = tableHtml;
    }
    return result;
}
