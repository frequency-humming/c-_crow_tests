#include "crow.h"
#include <array>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace std;

vector<string> msgs;
vector<pair<crow::response*, decltype(chrono::steady_clock::now())>> ress;

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

int main() {
    crow::SimpleApp app;

    // Start the traceroute in a separate thread and get a future for its result.
    std::future<std::string> tracerouteFuture;

    // Define your endpoint at the root directory.
    CROW_ROUTE(app, "/")
    ([] {
        auto page = crow::mustache::load("fancypage.html");
        crow::mustache::context ctx({{"person", "Robert"}});
        return page.render(ctx);
    });

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
