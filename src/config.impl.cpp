module;

#include <chrono>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <spdlog/spdlog.h>
#include <toml++/toml.hpp>

module config;

import utils;

namespace config
{
using namespace std::chrono_literals;

template <typename> struct is_std_vector : std::false_type
{
};
template <typename Item, typename Alloc> struct is_std_vector<std::vector<Item, Alloc>> : std::true_type
{
};

template <typename T> static T value_or(const toml::table &tbl, std::string_view key, T def)
{

    if constexpr (is_std_vector<T>::value)
    {
        using ElementType = typename T::value_type;

        if (const toml::array *arr = tbl[key].as_array())
        {
            T result;
            result.reserve(arr->size());
            for (const auto &elem : *arr)
            {
                if (auto v = elem.value<ElementType>())
                {
                    result.push_back(*v);
                }
            }
            return result;
        }
        return def;
    }
    else
    {
        if (auto v = tbl[key].value<T>())
            return *v;
        return def;
    }
}

LocalConfig ConfigLoader::load(std::string_view path)
{
    LocalConfig cfg{};
    spdlog::info("[config] loading file {}", path);
    auto tbl = toml::parse_file(path);

    if (auto p = tbl["poller"].as_table())
    {
        cfg.app.poller.node_base_url = value_or<std::string>(*p, "node_base_url", get_env_var("NODE_BASE_URL"));
        cfg.app.poller.scan_base_url = value_or<std::string>(*p, "scan_base_url", get_env_var("SCAN_BASE_URL"));

        cfg.app.poller.default_poll_offset_distance =
            value_or<std::uint32_t>(*p, "default_poll_offset_distance", cfg.app.poller.default_poll_offset_distance);
        cfg.app.poller.relax_offset_distance =
            value_or<std::uint32_t>(*p, "relax_offset_distance", cfg.app.poller.relax_offset_distance);
        cfg.app.poller.min_offset_distance =
            value_or<std::uint32_t>(*p, "min_offset_distance", cfg.app.poller.min_offset_distance);
        cfg.app.poller.max_offset_distance =
            value_or<std::uint32_t>(*p, "max_offset_distance", cfg.app.poller.max_offset_distance);
        cfg.app.poller.succesfull_attempts_before_double_distance =
            value_or<std::uint32_t>(*p, "succesfull_attempts_before_double_distance",
                                    cfg.app.poller.succesfull_attempts_before_double_distance);
        cfg.app.poller.relax_delay = std::chrono::milliseconds(
            value_or<std::uint64_t>(*p, "relax_delay_ms", cfg.app.poller.relax_delay.count()));
        cfg.app.poller.request_timeout = std::chrono::milliseconds(
            value_or<std::uint64_t>(*p, "request_timeout_ms", cfg.app.poller.request_timeout.count()));
        cfg.app.poller.template_ids =
            value_or<std::vector<std::string>>(*p, "template_ids", cfg.app.poller.template_ids);
        cfg.app.poller.interface_ids =
            value_or<std::vector<std::string>>(*p, "interface_ids", cfg.app.poller.interface_ids);
        cfg.app.poller.max_threads = value_or<std::size_t>(*p, "max_threads", cfg.app.poller.max_threads);
    }

    spdlog::info(
        "[config] poller node_base_url={} scan_base_url={} window=[{},{},{}] relax_delay_ms={} request_timeout_ms={}",
        cfg.app.poller.node_base_url, cfg.app.poller.scan_base_url, cfg.app.poller.min_offset_distance,
        cfg.app.poller.default_poll_offset_distance, cfg.app.poller.max_offset_distance,
        cfg.app.poller.relax_delay.count(), cfg.app.poller.request_timeout.count());

    if (auto p = tbl["parser"].as_table())
    {
        cfg.app.parser.max_threads = value_or<std::size_t>(*p, "max_threads", cfg.app.parser.max_threads);
    }

    if (auto p = tbl["queues"].as_table())
    {
        cfg.app.queues.raw_queue_capacity =
            value_or<std::size_t>(*p, "raw_queue_capacity", cfg.app.queues.raw_queue_capacity);
        cfg.app.queues.parsed_queue_capacity =
            value_or<std::size_t>(*p, "parsed_queue_capacity", cfg.app.queues.parsed_queue_capacity);
    }

    if (auto p = tbl["token_scan"].as_table())
    {
        cfg.app.token_scan.rescan_timeout = std::chrono::seconds(
            value_or<std::uint64_t>(*p, "rescan_timeout_s", cfg.app.token_scan.rescan_timeout.count()));
        cfg.app.token_scan.request_timeout = std::chrono::milliseconds(
            value_or<std::uint64_t>(*p, "request_timeout_ms", cfg.app.token_scan.request_timeout.count()));
        cfg.app.token_scan.page_size = value_or<size_t>(*p, "page_size", cfg.app.token_scan.page_size);
    }

    spdlog::info("[config] database target {}:{} db={} user={}", cfg.database.host, cfg.database.port,
                 cfg.database.name, cfg.database.user);

    if (auto p = tbl["service"].as_table())
    {
        cfg.app.service.host = value_or<std::string>(*p, "host", cfg.app.service.host);
        cfg.app.service.port = value_or<std::int32_t>(*p, "port", cfg.app.service.port);
    }

    return cfg;
}
} // namespace config
