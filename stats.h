#ifndef STATS_H
#define STATS_H

#include "crow.h"
#include <array>
#include <bitset>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::future<std::string> runTracerouteAsync(const std::string& endpoint);

class Stats {
    public:
        std::string cpuinfo{};
        std::string cpucount{};
        std::string cpucores{};
        std::string cputhreads{};
        std::string kernel{};
        std::string osinfo{};
        std::string osversion{};
        std::string hostname{};
        std::string uptime{};
        std::string memorytotal{};
        std::string memoryfree{};
        std::string memoryUsage{};
        std::string network{};
        std::string diskUsage{};
        std::string cpuUsage{};
        std::string disk{};
        std::vector<std::string> containerID{};
        std::vector<std::string> containerInfo{};
        inline static bool dockerFlag = false;
        static std::string agent;

        Stats() = default;
        static void setBoolean(bool flag) {
            dockerFlag = flag;
        }
        static bool getBoolean() {
            return dockerFlag;
        }
        static void setAgent(std::string_view a) {
            agent = a;
        }
        static std::string getAgent() {
            return agent;
        }
};

class DockerConfig {
    public:
        std::string containerid;
        std::string ipaddress;
        std::string name;
        std::string created;
        std::string logpath;
        std::string logtype;
        std::string port;
        std::string memory;
        std::string mount_source = "No Data";
        std::string mount_destination = "No Data";

        DockerConfig() = default;
};

std::string execCommand(const char* cmd, std::bitset<4> v);
void getStats(Stats& stats, crow::mustache::context& ctx);
void parseResponse(Stats& stats, crow::mustache::context& ctx);
void parseStats(Stats& stats, std::string& results, bool boolean, crow::mustache::context& ctx);
std::string addCpuUsage(Stats& stats);
void dockerHealth(Stats& stats);
void dockerStats(Stats& stats, std::vector<DockerConfig>& config);

#endif
