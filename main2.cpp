#include "crow.h"
#include <array>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>

using namespace std;

vector<string> msgs;
vector<pair<crow::response*, decltype(chrono::steady_clock::now())>> ress;

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

std::string execCommand(const char* cmd) {
    std::array<char, 200> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
        result += "<br>";
    }
    return result;
}

std::future<std::string> runTracerouteAsync(const std::string& endpoint) {
    return std::async(std::launch::async, [endpoint]() {
        try {
            std::string command = "traceroute -m 10 " + endpoint;
            return execCommand(command.c_str());
        } catch (const std::runtime_error& e) {
            std::cerr << "Traceroute Error: " << e.what() << std::endl;
            return "Traceroute failed: " + std::string(e.what());
        }
    });
}

std::vector<Stats> getStats() {
    std::vector<Stats> stats;
    stats.emplace_back("cpuInfo", execCommand("sysctl -n machdep.cpu.brand_string"));
    stats.emplace_back("osInfo", execCommand("sw_vers -productName"));
    stats.emplace_back("osVersion", execCommand("sw_vers -productVersion"));
    stats.emplace_back("hostname", execCommand("hostname"));
    stats.emplace_back("cpuCount", execCommand("sysctl -n hw.ncpu"));
    stats.emplace_back("cpuCores", execCommand("sysctl -n hw.physicalcpu"));
    stats.emplace_back("cpuThreads", execCommand("sysctl -n hw.logicalcpu"));
    stats.emplace_back("disk", execCommand("df -h"));
    stats.emplace_back("uptime", execCommand("uptime"));
    stats.emplace_back("cpuUsage", execCommand("top -l 1 | grep CPU"));
    stats.emplace_back("memoryUsage", execCommand("top -l 1 | grep PhysMem"));
    stats.emplace_back("diskUsage", execCommand("top -l 1 | grep Disk"));
    stats.emplace_back("networkUsage", execCommand("top -l 1 | grep Network"));
    std::string memory = execCommand("sysctl hw.memsize");
    std::regex non_digit("[^0-9]");
    std::string only_digits = std::regex_replace(memory, non_digit, "");
    long long number = std::stoll(only_digits) / 1024 / 1024 / 1024;
    std::string ram = std::to_string(number) + "GB";
    stats.emplace_back("memory", ram);
    return stats;
}

int main() {
    crow::SimpleApp app;
    std::future<std::string> tracerouteFuture;
    std::vector<Stats> stats = getStats();

    CROW_ROUTE(app, "/")
    ([&stats] {
        crow::mustache::context ctx;
        for (const auto& stat : stats) {
            ctx[stat.getName()] = stat.getValue();
        }
        auto page = crow::mustache::load("home.html");
        return page.render(ctx);
    });

    // CROW_ROUTE(app, "/stats")
    // ([&stats] {
    //     crow::json::wvalue x;
    //     for (auto& stat : stats) {
    //         x[stat.getName()] = stat.getValue();
    //     }
    //     return x;
    // });

    // Serve the result of traceroute on the /test endpoint.
    CROW_ROUTE(app, "/test")
    ([] {
        auto page = crow::mustache::load("test.html");
        return page.render();
    });

    CROW_ROUTE(app, "/send").methods("POST"_method)([&tracerouteFuture](const crow::request& req) {
        std::string endpoint = req.body; // Assuming the endpoint is sent as plain text in the body
        tracerouteFuture = runTracerouteAsync(endpoint); // Run traceroute asynchronously

        return crow::response(202); // HTTP 202 Accepted: Request accepted and processing
    });

    CROW_ROUTE(app, "/result")
    ([&tracerouteFuture] {
        if (tracerouteFuture.valid() &&
            tracerouteFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            // If the future is ready, get the result and return it
            return tracerouteFuture.get();
        } else {
            // If the future is not ready, inform the client the result is still pending
            return std::string("Traceroute result is still pending...");
        }
    });

    app.port(18080).multithreaded().run();
}
