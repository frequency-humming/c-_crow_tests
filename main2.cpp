#include "crow.h"
#include "stats.h"

std::vector<std::string> msgs;

int main() {
    crow::SimpleApp app;
    std::future<std::string> tracerouteFuture;
    std::vector<Stats> stats = getStats();
    parseResponse(stats);
#ifndef __APPLE__
    std::string command{addCpuUsage(stats)};
#endif
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
    CROW_ROUTE(app, "/stats").methods("POST"_method)([]() {
        crow::mustache::context ctx;
        ctx["cpuUsage"] = execCommand("top -l 1 | grep CPU", std::bitset<4>{0b0100});
        ctx["memoryUsage"] = execCommand("top -l 1 | grep PhysMem", std::bitset<4>{0b0000});
        ctx["diskUsage"] = execCommand("top -l 1 | grep Disk", std::bitset<4>{0b0100});
        ctx["networkUsage"] = execCommand("top -l 1 | grep Network", std::bitset<4>{0b0000});
        return ctx;
    });
#else
    CROW_ROUTE(app, "/memory").methods("POST"_method)([&command]() {
        crow::mustache::context ctx;
        ctx["cpuUsage"] = execCommand(command.c_str(), std::bitset<4>{0b0010});
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

    // app.port(18080).multithreaded().run();
    app.port(18080).concurrency(2).run();
}
