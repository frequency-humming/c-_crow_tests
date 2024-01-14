#ifndef STATS_H
#define STATS_H

#include <array>
#include <bitset>
#include <future>
#include <string>
#include <vector>

std::future<std::string> runTracerouteAsync(const std::string& endpoint);
std::string execCommand(const char* cmd, std::bitset<4> v);

class Stats {
    private:
        std::string name{};
        std::string value{};

    public:
        Stats(std::string_view name, std::string_view value) : name{name}, value{value} {
        }
        std::string getName() const {
            return name;
        }
        std::string getValue() const {
            return value;
        }
};

std::vector<Stats> getStats();
void parseResponse(std::vector<Stats>& stats);
std::string addCpuUsage(std::vector<Stats>& stats);

#endif
