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

    CROW_ROUTE(app, "/test")
    ([] {
        auto page = crow::mustache::load("test.html");
        return page.render();
    });

    CROW_ROUTE(app, "/send").methods("POST"_method)([&tracerouteFuture](const crow::request& req) {
        std::string endpoint = req.body;
        tracerouteFuture = runTracerouteAsync(endpoint);
        return crow::response(202);
    });

    CROW_ROUTE(app, "/result")
    ([&tracerouteFuture] {
        if (tracerouteFuture.valid() && tracerouteFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            return tracerouteFuture.get();
        } else {
            return std::string("Traceroute result is still pending...");
        }
    });

    app.port(18080).multithreaded().run();
}
