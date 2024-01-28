#ifndef STATS_H
#define STATS_H

#include <array>
#include <bitset>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::future<std::string> runTracerouteAsync(const std::string& endpoint);

class Stats {
    private:
        std::string name{};
        std::string value{};
        inline static bool dockerFlag = false;
        static std::string agent;

    public:
        Stats(std::string_view name, std::string_view value) : name{name}, value{value} {
        }
        std::string getName() const {
            return name;
        }
        std::string getValue() const {
            return value;
        }
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
        DockerConfig(std::string_view containerid,
                     std::string_view ipaddress,
                     std::string_view name,
                     std::string_view created,
                     std::string_view logpath,
                     std::string_view port,
                     std::string_view porttype,
                     std::string_view memory,
                     std::string_view mount_source,
                     std::string_view mount_destination)
            : containerid{containerid}, ipaddress{ipaddress}, name{name}, created{created}, logpath{logpath}, logtype{porttype}, port{port}, memory{memory},
              mount_source{mount_source}, mount_destination{mount_destination} {};
};

std::string execCommand(const char* cmd, std::bitset<4> v);
void getStats(std::vector<Stats>& stats);
void parseResponse(std::vector<Stats>& stats);
std::string addCpuUsage(std::vector<Stats>& stats);
void dockerHealth(std::vector<Stats>& stats);
void dockerStats(std::vector<Stats>& stats, std::vector<DockerConfig>& config);

#endif
