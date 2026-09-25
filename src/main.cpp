#include "httplib.h"
#include "json.hpp"
#include "policy_engine.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    using json = nlohmann::json;
    httplib::Server server;
    PolicyEngine engine;
    server.set_mount_point("/", "./web");
    server.Get("/api/state", [&](const httplib::Request&, httplib::Response& response) {
        response.set_content(engine.getSystemState().dump(), "application/json; charset=utf-8");
    });
    server.Post("/api/backtest", [&](const httplib::Request& request, httplib::Response& response) {
        const json payload = json::parse(request.body);
        const std::string strategy = payload.value("strategy", std::string("all"));
        const std::string ticker = payload.value("ticker", std::string("ACME_TECH"));
        response.set_content(engine.runBacktest(strategy, ticker).dump(), "application/json; charset=utf-8");
    });
    server.Post("/api/reset", [&](const httplib::Request&, httplib::Response& response) {
        engine.resetDefaults();
        response.set_content("{\"status\":\"reset_ok\"}", "application/json; charset=utf-8");
    });

    const char* port_env = std::getenv("PORT");
    const int port = port_env ? std::atoi(port_env) : 3000;
    std::cout << "[TradeSim] Server listening on http://0.0.0.0:" << port << std::endl;
    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "[TradeSim] Unable to bind preview port " << port << '\n';
        return 1;
    }
    return 0;
}
