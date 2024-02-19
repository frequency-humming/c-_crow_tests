#include "crow.h"
#include "stats.h"
#include <regex>

void parseIP(Metrics& metric) {
    for (const auto& pair : metric.ip) {
        if (pair.second >= 10 && metric.info[pair.first].country.empty()) {
            std::string value;
            MetricDetails details;
            std::string cmd = "curl -s -S https://ipinfo.io/" + pair.first + "?token=" + Config::token;
            std::string results = execCommand(cmd.c_str(), std::bitset<4>{0b0000});
            std::istringstream iss(results);
            while (std::getline(iss, value)) {
                if (value.find("host") != std::string::npos) {
                    details.hostname = value.substr(value.find(':') + 1);
                } else if (value.find("city") != std::string::npos) {
                    details.city = value.substr(value.find(':') + 1);
                } else if (value.find("region") != std::string::npos) {
                    details.region = value.substr(value.find(':') + 1);
                } else if (value.find("country") != std::string::npos) {
                    details.country = value.substr(value.find(':') + 1);
                } else if (value.find("org") != std::string::npos) {
                    details.org = value.substr(value.find(':') + 1);
                }
            }
            metric.info[pair.first] = details;
        }
    }
}
void parseResponse(Stats& stats, std::string& memory, crow::mustache::context& ctx) {
    long number{};
    std::regex non_digit("[^0-9]");
    std::string only_digits = std::regex_replace(memory, non_digit, "");
    if (!only_digits.empty()) {
        number = std::stoll(only_digits) / 1024 / 1024 / 1024;
        std::string ram = std::to_string(number) + " GB";
        stats.memorytotal = ram;
        ctx["memorytotal"] = stats.memorytotal;
    } else {
        std::cerr << " Only digits is empty or invalid " << only_digits << std::endl;
        stats.memorytotal = memory;
        ctx["memorytotal"] = stats.memorytotal;
    }
}

void parseStats(Stats& stats, std::string& results, bool boolean, crow::mustache::context& ctx) {
    std::istringstream iss(results);
    int count = 0;
    std::string value;
    if (boolean) {
        while (std::getline(iss, value)) {
            switch (count) {
                case 0:
                    stats.cpuinfo = value;
                    ctx["cpuInfo"] = stats.cpuinfo;
                    break;
                case 1:
                    stats.cpucount = value;
                    ctx["cpuCount"] = stats.cpucount;
                    break;
                case 2:
                    stats.cpucores = value;
                    ctx["cpuCores"] = stats.cpucores;
                    break;
#ifdef __APPLE__
                case 3:
                    stats.cputhreads = value;
                    ctx["cpuThreads"] = stats.cputhreads;
                    break;
                case 4:
                    parseResponse(stats, value, ctx);
                    break;
#else
                case 3:
                    stats.kernel = value;
                    ctx["kernel"] = stats.kernel;
                    break;
                case 4:
                    stats.memoryfree = value;
                    ctx["memoryfree"] = stats.memoryfree;
                    break;
#endif
                case 5:
                    stats.osinfo = value;
                    ctx["osInfo"] = stats.osinfo;
                    break;
                case 6:
                    stats.osversion = value;
                    ctx["osVersion"] = stats.osversion;
                    break;
                case 7:
                    stats.hostname = value;
                    ctx["hostname"] = stats.hostname;
                    break;
                case 8:
                    stats.uptime = value;
                    ctx["uptime"] = stats.uptime;
                    break;
#ifndef __APPLE__
                case 9:
                    stats.memorytotal = value;
                    ctx["memorytotal"] = stats.memorytotal;
                    break;
#endif
#ifdef __APPLE__
                case 9:
                    stats.memoryUsage = value;
                    ctx["memoryUsage"] = stats.memoryUsage;
                    break;
                case 10:
                    stats.network = value;
                    ctx["network"] = stats.network;
                    break;
                case 11:
                    stats.cpuUsage = value;
                    ctx["cpuUsage"] = stats.cpuUsage;
                    break;
                case 12:
                    stats.diskUsage = value;
                    ctx["diskUsage"] = stats.diskUsage;
                    break;
#endif
            }
            count++;
        }
    } else {
        while (std::getline(iss, value)) {
            switch (count) {
                case 0:
                    stats.uptime = value;
                    ctx["uptime"] = stats.uptime;
                    break;
                case 1:
                    stats.memoryUsage = value;
                    ctx["memoryUsage"] = stats.memoryUsage;
                    break;
                case 2:
                    stats.network = value;
                    ctx["network"] = stats.network;
                    break;
                case 3:
                    stats.cpuUsage = value;
                    ctx["cpuUsage"] = stats.cpuUsage;
                    break;
                case 4:
                    stats.diskUsage = value;
                    ctx["diskUsage"] = stats.diskUsage;
                    break;
            }
            count++;
        }
    }
}
