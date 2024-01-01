#include "stats.h"
#include <iostream>
#include <iterator>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

std::vector<Stats> getStats() {
    std::vector<Stats> stats;
#ifdef __APPLE__
    stats.emplace_back("cpuInfo", execCommand("sysctl -n machdep.cpu.brand_string", std::bitset<4>{0b0000}));
    stats.emplace_back("osInfo", execCommand("sw_vers -productName", std::bitset<4>{0b0000}));
    stats.emplace_back("osVersion", execCommand("sw_vers -productVersion", std::bitset<4>{0b0000}));
    stats.emplace_back("hostname", execCommand("hostname", std::bitset<4>{0b0000}));
    stats.emplace_back("cpuCount", execCommand("sysctl -n hw.ncpu", std::bitset<4>{0b0000}));
    stats.emplace_back("cpuCores", execCommand("sysctl -n hw.physicalcpu", std::bitset<4>{0b0000}));
    stats.emplace_back("cpuThreads", execCommand("sysctl -n hw.logicalcpu", std::bitset<4>{0b0000}));
    stats.emplace_back("disk", execCommand("df -h", std::bitset<4>{0b0010}));
    stats.emplace_back("uptime", execCommand("uptime", std::bitset<4>{0b0000}));
    stats.emplace_back("cpuUsage", execCommand("top -l 1 | grep CPU", std::bitset<4>{0b0100}));
    stats.emplace_back("memoryUsage", execCommand("top -l 1 | grep PhysMem", std::bitset<4>{0b0000}));
    stats.emplace_back("diskUsage", execCommand("top -l 1 | grep Disk", std::bitset<4>{0b0100}));
    stats.emplace_back("networkUsage", execCommand("top -l 1 | grep Network", std::bitset<4>{0b0000}));
#else
    stats.emplace_back("cpuInfo", execCommand("cat /proc/cpuinfo | grep 'model name' | uniq | awk -F: '{print $2}'", std::bitset<4>{0b0000}));
    stats.emplace_back("osInfo", execCommand("cat /etc/os-release | grep '^NAME=' | awk -F= '{print $2}'", std::bitset<4>{0b0000}));
    stats.emplace_back("osVersion", execCommand("cat /etc/os-release | grep VERSION_ID | awk -F= '{print $2}'", std::bitset<4>{0b0000}));
    stats.emplace_back("hostname", execCommand("hostname", std::bitset<4>{0b0000}));
    stats.emplace_back("cpuCount", execCommand("cat /proc/cpuinfo | grep 'processor' | wc -l", std::bitset<4>{0b0000}));
    stats.emplace_back("cpuCores", execCommand("nproc --all", std::bitset<4>{0b0000}));
    stats.emplace_back("disk", execCommand("df -h", std::bitset<4>{0b0010}));
    stats.emplace_back("uptime", execCommand("uptime", std::bitset<4>{0b0000}));
    stats.emplace_back("cpuUsage", execCommand("mpstat -P ALL 1 | head -n 8 |  awk '/(4 CPU)/ {found=1; next} found'", std::bitset<4>{0b0010}));
    stats.emplace_back("memoryUsage", execCommand("cat /proc/meminfo | grep 'MemTotal' | awk -F: '{print $2}'", std::bitset<4>{0b0000}));
    stats.emplace_back("networkUsage", execCommand("ifconfig", std::bitset<4>{0b1000}));
#endif
    return stats;
}

void parseResponse(std::vector<Stats>& stats) {
#ifdef __APPLE__
    std::string memory = execCommand("sysctl hw.memsize", std::bitset<4>{0b0000});
    std::regex non_digit("[^0-9]");
    std::string only_digits = std::regex_replace(memory, non_digit, "");
    long number = std::stoll(only_digits) / 1024 / 1024 / 1024;
    std::string ram = std::to_string(number) + "GB";
    stats.emplace_back("memory", ram);
#else
    stats.emplace_back("memory", execCommand("free -m", std::bitset<4>{0b0110}));
#endif
}

std::future<std::string> runTracerouteAsync(const std::string& endpoint) {
    return std::async(std::launch::async, [endpoint]() {
        try {
            std::string command = "traceroute -m 10 " + endpoint;
            return execCommand(command.c_str(), std::bitset<4>{0b0001});
        } catch (const std::runtime_error& e) {
            std::cerr << "Traceroute Error: " << e.what() << std::endl;
            return "Traceroute failed: " + std::string(e.what());
        }
    });
}

std::string execCommand(const char* cmd, std::bitset<4> v) {
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
                    } else if (var && !v.test(2)) {
                        mountPoint += " " + part;
                        tableHtml += "<td>" + mountPoint + "</td>";
                        var = false;
                    } else if (v.test(2) && !var) {
                        tableHtml += "<td>#</td>";
                        tableHtml += "<td>" + part + "</td>";
                        var = true;
                    } else {
                        tableHtml += "<td>" + part + "</td>";
                    }
                }
                tableHtml += "</tr>";
            } else if (v.test(2)) {
                result = buffer.data();
                return result;
            } else if (v.test(3)) {
                result += "<div>";
                result += buffer.data();
                result += "</div>";
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
