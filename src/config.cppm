module;

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

export module config;

import utils;

export namespace config
{
using namespace std::chrono_literals;

struct PollerConfig
{
    std::string node_base_url{};
    std::string scan_base_url{};
    std::uint32_t default_poll_offset_distance{1024};
    std::uint32_t relax_offset_distance{256};
    std::uint32_t min_offset_distance{1};
    std::uint32_t max_offset_distance{4096};
    std::uint32_t succesfull_attempts_before_double_distance{10};
    std::chrono::milliseconds relax_delay{1s};
    std::chrono::milliseconds request_timeout{30s};
    std::vector<std::string> template_ids;
    std::vector<std::string> interface_ids;
    std::size_t max_threads{2};
};

struct ParserConfig
{
    std::size_t max_threads{2};
};

struct QueueConfig
{
    std::size_t raw_queue_capacity{10};
    std::size_t parsed_queue_capacity{10};
};

struct DatabaseConfig
{
    std::string host = get_env_var("DB_HOST");
    std::uint16_t port = std::stoi(get_env_var("DB_PORT"));
    std::string user = get_env_var("DB_USER");
    std::string password = get_env_var("DB_PASSWORD");
    std::string name = get_env_var("DB_NAME");
};

struct ScanConfig
{
    std::string utility_base_url = get_env_var("UTILITY_BASE_URL");
    std::chrono::seconds rescan_timeout{1h};
    std::chrono::milliseconds request_timeout{30s};
    std::size_t page_size{100};
};

struct ServiceConfig
{
    std::string host = get_env_var("SERVICE_HOST");
    std::int32_t port = std::stoi(get_env_var("SERVICE_PORT"));
};

struct AppConfig
{
    PollerConfig poller;
    ParserConfig parser;
    QueueConfig queues;
    ScanConfig token_scan;
    ServiceConfig service;
};

struct LocalConfig
{
    AppConfig app;
    DatabaseConfig database;
};

class ConfigLoader
{
  public:
    static LocalConfig load(std::string_view path);
};
} // namespace config
