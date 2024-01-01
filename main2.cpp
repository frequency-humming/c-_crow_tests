#include "crow.h"
#include "stats.h"

std::vector<std::string> msgs;

int main() {
    crow::SimpleApp app;
    std::future<std::string> tracerouteFuture;
    std::vector<Stats> stats = getStats();
    parseResponse(stats);

    CROW_ROUTE(app, "/")
    ([&stats] {
        crow::mustache::context ctx;
        for (const auto& stat : stats) {
            ctx[stat.getName()] = stat.getValue();
        }
#ifdef __APPLE__
        auto page = crow::mustache::load("home.html");
#else
        auto page = crow::mustache::load("home_linux.html");
#endif
        return page.render(ctx);
    });

    CROW_ROUTE(app, "/tools")
    ([] {
        auto page = crow::mustache::load("trace.html");
        return page.render();
    });

    CROW_ROUTE(app, "/sendendpoint").methods("POST"_method)([&tracerouteFuture](const crow::request& req) {
        std::string endpoint = req.body;
        tracerouteFuture = runTracerouteAsync(endpoint);
        return crow::response(202);
    });
#ifdef __APPLE__
    CROW_ROUTE(app, "/stats").methods("POST"_method)([&stats]() {
        crow::mustache::context ctx;
        for (const auto& stat : stats) {
            if (stat.getName() == "cpuUsage") {
                ctx[stat.getName()] = execCommand("top -l 1 | grep CPU", std::bitset<4>{0b0100});
            } else if (stat.getName() == "memoryUsage") {
                ctx[stat.getName()] = execCommand("top -l 1 | grep PhysMem", std::bitset<4>{0b0000});
            } else if (stat.getName() == "diskUsage") {
                ctx[stat.getName()] = execCommand("top -l 1 | grep Disk", std::bitset<4>{0b0100});
            } else if (stat.getName() == "networkUsage") {
                ctx[stat.getName()] = execCommand("top -l 1 | grep Network", std::bitset<4>{0b0000});
            } else {
                continue;
            }
        }
        return ctx;
    });
#else
    CROW_ROUTE(app, "/memory").methods("POST"_method)([&stats]() {
        crow::mustache::context ctx;
        for (const auto& stat : stats) {
            if (stat.getName() == "cpuUsage") {
                ctx[stat.getName()] = execCommand("mpstat -P ALL 1 | head -n 8 |  awk '/(4 CPU)/ {found=1; next} found'", std::bitset<4>{0b0010});
                break;
            }
        }
        return ctx;
    });
#endif

    CROW_ROUTE(app, "/results")
    ([&tracerouteFuture] {
        if (tracerouteFuture.valid() && tracerouteFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            return tracerouteFuture.get();
        } else {
            return std::string("Traceroute result is still pending...");
        }
    });

    app.port(18080).multithreaded().run();
}
