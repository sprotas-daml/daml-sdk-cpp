module;
#include <future>
#include <crow.h>
#include <crow/routing.h>
#include <cstdint>

export module api:core;

import database;

export namespace api
{
using Router = std::shared_ptr<crow::Blueprint>;
using InitRouteFunction = std::function<Router(std::shared_ptr<db::Connection> m_conn)>;
class Server
{
  public:
    Server(std::shared_ptr<db::Connection> conn);

    void setup_api_routes(const std::vector<InitRouteFunction> &init_route_functions);

    void start_async(const std::string &host, std::int32_t port);

    void stop();

  private:
    crow::SimpleApp m_app;
    std::vector<Router> routers;
    Router api_router;
    std::shared_ptr<db::Connection> m_conn;
    std::future<void> m_future;
    std::string m_host;
    std::int32_t m_port{};
};
} // namespace api

