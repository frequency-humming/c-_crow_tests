#include "crow.h"
#include "stats.h"

int main() {
    crow::SimpleApp app;
    Stats stats;
    Metrics metrics;
    std::vector<DockerConfig> config;
    std::string command;
    std::future<void> metricsFuture;
    CROW_ROUTE(app, "/")
    ([&stats, &command, &metrics, &metricsFuture] {
        metricsFuture = std::async(std::launch::async, [&metrics] { getMetrics(metrics); });
        crow::mustache::context ctx;
        std::vector<std::string> details;
        Stats::setBoolean(false);
        stats.containerID.clear();
        stats.containerInfo.clear();
        getStats(stats, ctx);
        dockerHealth(stats);
#ifndef __APPLE__
        command = addCpuUsage(stats);
        ctx["cpuUsage"] = stats.cpuUsage;
#endif
        bool hasContainers = false;
        int count = 0;
        if (!stats.containerID.empty()) {
            hasContainers = true;
            count = stats.containerID.size();
            ctx["docker"] = count;
        }
        if (!stats.containerInfo.empty()) {
            for (auto& stat : stats.containerInfo) {
                details.emplace_back(stat);
            }
        }
        ctx["disk"] = stats.disk;
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

    CROW_ROUTE(app, "/metrics")
    ([&metrics, &metricsFuture] {
        crow::mustache::context ctx;
        std::vector<crow::json::wvalue> containers;
        if (metricsFuture.valid() && metricsFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            for (auto& pair : metrics.ip) {
                if (pair.second >= 10) {
                    crow::json::wvalue ctxMetric;
                    ctxMetric["metric"] = "IP : " + pair.first + " Count: " + std::to_string(pair.second);
                    ctxMetric["hostname"] = metrics.info[pair.first].hostname;
                    ctxMetric["city"] = metrics.info[pair.first].city;
                    ctxMetric["region"] = metrics.info[pair.first].region;
                    ctxMetric["country"] = metrics.info[pair.first].country;
                    ctxMetric["org"] = metrics.info[pair.first].org;
                    ctxMetric["date"] = metrics.dates[pair.first];
                    containers.emplace_back(ctxMetric);
                }
            }
        } else {
            crow::json::wvalue ctxMetric;
            ctxMetric["metric"] = "No Data ";
            containers.emplace_back(ctxMetric);
        }
        ctx["containers"] = crow::json::wvalue(containers);
        auto page = crow::mustache::load("metrics.html");
        return page.render(ctx);
    });

#ifdef __APPLE__
    CROW_ROUTE(app, "/stats").methods("POST"_method)([&stats]() {
        crow::mustache::context ctx;
        std::string result =
            execCommand("uptime && top -l 1 | awk '/PhysMem/{physMem=$0} /Network/{network=$0} /CPU/ && !cpu {cpu=$0} /Disk/ && !disk {disk=$0} END{print "
                        "physMem; print network; print cpu; print disk}'",
                        std::bitset<4>{0b0000});
        parseStats(stats, result, false, ctx);
        return ctx;
    });
#else
    CROW_ROUTE(app, "/memory").methods("POST"_method)([&command]() {
        crow::mustache::context ctx;
        ctx["cpuUsage"] = execCommand(command.c_str(), std::bitset<4>{0b0010});
        ctx["uptime"] = execCommand("uptime", std::bitset<4>{0b0000});
        ctx["disk"] = execCommand("df -h", std::bitset<4>{0b0010});
        ctx["memoryfree"] = execCommand("cat /proc/meminfo | grep 'MemFree' | awk -F': ' '{print $2/1024}'", std::bitset<4>{0b0010});
        return ctx;
    });
#endif

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
