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
        auto page = crow::mustache::load("home.html");
        return page.render(ctx);
    });

    CROW_ROUTE(app, "/test")
    ([] {
        auto page = crow::mustache::load("test.html");
        return page.render();
    });

    CROW_ROUTE(app, "/send").methods("POST"_method)([&tracerouteFuture](const crow::request& req) {
        std::string endpoint = req.body;                 // Assuming the endpoint is sent as plain text in the body
        tracerouteFuture = runTracerouteAsync(endpoint); // Run traceroute asynchronously

        return crow::response(202); // HTTP 202 Accepted: Request accepted and processing
    });

    CROW_ROUTE(app, "/result")
    ([&tracerouteFuture] {
        if (tracerouteFuture.valid() && tracerouteFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            // If the future is ready, get the result and return it
            return tracerouteFuture.get();
        } else {
            // If the future is not ready, inform the client the result is still pending
            return std::string("Traceroute result is still pending...");
        }
    });

    app.port(18080).multithreaded().run();
}
