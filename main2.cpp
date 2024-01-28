#include "crow.h"
#include "stats.h"

std::vector<std::string> msgs;

int main() {
    crow::SimpleApp app;
    std::future<std::string> tracerouteFuture;
    std::vector<Stats> stats;
    std::vector<DockerConfig> config;
    CROW_ROUTE(app, "/")
    ([&stats] {
        crow::mustache::context ctx;
        std::vector<std::string> details;
        stats.clear();
        Stats::setBoolean(false);
        getStats(stats);
        parseResponse(stats);
        dockerHealth(stats);
#ifndef __APPLE__
        std::string command{addCpuUsage(stats)};
#endif
        bool hasContainers = false;
        int count = 0;
        for (const auto& stat : stats) {
            if (stat.getName() == "containerId") {
                hasContainers = true;
                count++;
                ctx["docker"] = count;
            } else if (stat.getName() == "name") {
                details.emplace_back(stat.getValue());
            } else {
                ctx[stat.getName()] = stat.getValue();
            }
        }
        ctx["hasContainers"] = hasContainers;
        if (hasContainers) {
            ctx["containers"] = details;
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
        ctx["uptime"] = execCommand("uptime", std::bitset<4>{0b0000});
        ctx["memoryUsage"] = execCommand("top -l 1 | grep PhysMem", std::bitset<4>{0b0000});
        ctx["diskUsage"] = execCommand("top -l 1 | grep Disk", std::bitset<4>{0b0100});
        ctx["networkUsage"] = execCommand("top -l 1 | grep Network", std::bitset<4>{0b0000});
        return ctx;
    });
#else
    CROW_ROUTE(app, "/memory").methods("POST"_method)([&command]() {
        crow::mustache::context ctx;
        ctx["cpuUsage"] = execCommand(command.c_str(), std::bitset<4>{0b0010});
        ctx["uptime"] = execCommand("uptime", std::bitset<4>{0b0000});
        ctx["disk"] = execCommand("df -h", std::bitset<4>{0b0010});
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

    CROW_ROUTE(app, "/docker")
    ([&stats, &config] {
        crow::mustache::context ctx;
        std::vector<crow::json::wvalue> containers;
        config.clear();
        if (Stats::getBoolean()) {
            dockerStats(stats, config);
            for (const auto& stat : config) {
                crow::json::wvalue ctxContainer;
                ctxContainer["containerId"] = stat.containerid;
                ctxContainer["name"] = stat.name;
                ctxContainer["created"] = stat.created;
                ctxContainer["ip"] = stat.ipaddress;
                ctxContainer["logPath"] = stat.logpath;
                ctxContainer["logType"] = stat.logtype;
                ctxContainer["port"] = stat.port;
                ctxContainer["memory"] = stat.memory;
                ctxContainer["mountDestination"] = stat.mount_destination;
                ctxContainer["mountSource"] = stat.mount_source;
                containers.emplace_back(ctxContainer);
            }
            ctx["hasContainers"] = true;
            ctx["containers"] = crow::json::wvalue(containers);
        } else {
            ctx["docker"] = "Docker is not running";
            ctx["noContainers"] = true;
        }
        auto page = crow::mustache::load("docker.html");
        return page.render(ctx);
    });

    // app.port(18080).multithreaded().run();
    app.port(18080).concurrency(2).run();
}
