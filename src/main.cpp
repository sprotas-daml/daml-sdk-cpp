#include <string>

#include <csignal>
#include <fmt/format.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

import utils;
import app;
import config;

import daml.crypto;
import daml.api;

volatile std::sig_atomic_t g_shutdown_requested = 0;

void signal_handler(int signal)
{
    g_shutdown_requested = 1;
}

void init_logger()
{
    using namespace std::chrono_literals;

    spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    const auto logger = std::make_shared<spdlog::async_logger>("async_logger", stdout_sink, spdlog::thread_pool(),
                                                               spdlog::async_overflow_policy::block);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    constexpr auto current_level = static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);

    spdlog::set_level(current_level);
    if (current_level == spdlog::level::trace || current_level == spdlog::level::debug)
    {
        spdlog::flush_on(current_level);
        spdlog::flush_every(1s);
    }
    else
    {
        spdlog::flush_on(spdlog::level::err);
        spdlog::flush_every(10s);
    }
}

int main()
{
    using namespace std::chrono_literals;
    using namespace daml::crypto;
    using namespace daml::api;

    std::signal(SIGINT, signal_handler);  // Ctrl+C
    std::signal(SIGTERM, signal_handler); // systemctl stop or docker stop
    std::signal(SIGKILL, signal_handler);

    try
    {
        init_logger();

        {
            std::string token;
            try
            {
                token = get_env_var("TOKEN");
                spdlog::info(token);
            }
            catch (...)
            {
            }
            std::lock_guard lock(token::token_manager_mutex);
            token::token_manager_inst = {token::TokenManager(token::TokenManager::Secrets{
                .base_url = get_env_var("TOKEN_BASE_URL"),
                .client_id = get_env_var("TOKEN_CLIENT_ID"),
                .client_secret = get_env_var("TOKEN_CLIENT_SECRET"),
                .audience = get_env_var("TOKEN_AUDIENCE"),
                .key = get_env_var("TOKEN_KEY"),
                .token = token,
            })};
        }

        std::string cfg_path;
        try
        {
            cfg_path = get_env_var("LOCAL_NODE_CONFIG");
        }
        catch (const std::exception &e)
        {
            cfg_path = std::string("config.local.toml");
        }
        spdlog::info("[main] loading config from {}", cfg_path);
        const auto cfg = config::ConfigLoader::load(cfg_path);

        registry::Registry::init({.url = cfg.app.poller.node_base_url, .timeout = cfg.app.poller.request_timeout},
                                 {.url = cfg.app.poller.scan_base_url, .timeout = cfg.app.poller.request_timeout});
        request::check_and_grant_rights(token::get_token(), token::get_user_id());

        const std::string conn_str =
            fmt::format("postgresql://{}:{}@{}:{}/{}", cfg.database.user, cfg.database.password, cfg.database.host,
                        cfg.database.port, cfg.database.name);
        spdlog::info("[main] db target {}:{} as {}", cfg.database.host, cfg.database.port, cfg.database.user);

        app::Runtime app(conn_str, cfg.app);
        app.start();

        while (g_shutdown_requested == 0)
        {
            std::this_thread::sleep_for(200ms);
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("[main] fatal: {}", e.what());
        spdlog::shutdown();
        return 1;
    }

    spdlog::shutdown();
    return 0;
}
