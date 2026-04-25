module;
#include <crow/app.h>
#include <crow/routing.h>
#include <cstdint>
#include <future>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

module api;

namespace api
{

Server::Server(std::shared_ptr<db::Connection> conn)
    : m_conn(conn), api_router(std::make_shared<crow::Blueprint>("api"))
{
}

void Server::setup_api_routes(const std::vector<InitRouteFunction> &init_route_functions)
{
    routers.reserve(init_route_functions.size());
    for (auto &init_route : init_route_functions)
    {
        auto route = init_route(m_conn);
        api_router->register_blueprint(*route);
        routers.push_back(route);
    }
    m_app.register_blueprint(*api_router);

    CROW_ROUTE(m_app, "/test")([]() { return crow::response(200); });
}

void Server::start_async(const std::string &host, std::int32_t port)
{
    m_host = host;
    m_port = port;
    m_future = std::async(std::launch::async, [this]() {
        spdlog::info("[local api] starting on {}:{}", m_host, m_port);
        m_app.bindaddr(m_host).port(m_port).multithreaded().run();
    });
}

void Server::stop()
{
    m_app.stop();
    if (m_future.valid())
    {
        m_future.get();
    }
}
} // namespace api
