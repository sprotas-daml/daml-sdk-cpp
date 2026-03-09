module;

#include <cpr/cpr.h>
#include <format>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

export module daml.api:client;

import :registry;
import :ssl_connection;

export namespace daml::api::client
{
using registry::NodeConfig;
using registry::Registry;

nlohmann::json post(const NodeConfig &config, std::string_view path, const std::string &token,
                    const nlohmann::json &body)
{
    ssl_connection::SslConnection conn(config.url, config.timeout);
    conn.append_path(path);

    std::string target_url = conn.target();
    spdlog::info("New POST request to {}", target_url);
    spdlog::debug("Payload: {}", body.dump());

    cpr::Session &session = conn.getSession();

    session.SetHeader(cpr::Header{{"Accept", "application/json"},
                                  {"Content-Type", "application/json"},
                                  {"Authorization", std::format("Bearer {}", token)}});
    session.SetBody(cpr::Body{body.dump()});

    cpr::Response response = session.Post();

    if (response.error.code != cpr::ErrorCode::OK)
    {
        throw std::runtime_error(std::format("Transport error to {}: {}", target_url, response.error.message));
    }

    if (response.status_code < 200 || response.status_code >= 300)
    {
        throw std::runtime_error(
            std::format("Post to {} failed with status {}: {}", target_url, response.status_code, response.text));
    }

    return nlohmann::json::parse(response.text);
}

nlohmann::json get(const NodeConfig &config, std::string_view path, const std::string &token)
{
    ssl_connection::SslConnection conn(config.url, config.timeout);
    conn.append_path(path);

    std::string target_url = conn.target();
    spdlog::info("New GET request to {}", target_url);

    cpr::Session &session = conn.getSession();

    session.SetHeader(cpr::Header{{"Accept", "application/json"}, {"Authorization", std::format("Bearer {}", token)}});

    cpr::Response response = session.Get();

    if (response.error.code != cpr::ErrorCode::OK)
    {
        throw std::runtime_error(std::format("GET transport error to {}: {}", target_url, response.error.message));
    }

    if (response.status_code != 200)
    {
        throw std::runtime_error(
            std::format("GET to {} failed with status {}: {}", target_url, response.status_code, response.text));
    }

    return nlohmann::json::parse(response.text);
}

nlohmann::json ledger_get(std::string_view path, const std::string &token)
{
    return get(Registry::get_ledger(), path, token);
}
nlohmann::json scan_get(std::string_view path, const std::string &token)
{
    return get(Registry::get_scan(), path, token);
}
nlohmann::json ledger_post(std::string_view path, const std::string &token, const nlohmann::json &body)
{
    return post(Registry::get_ledger(), path, token, body);
}
nlohmann::json scan_post(std::string_view path, const std::string &token, const nlohmann::json &body)
{
    return post(Registry::get_scan(), path, token, body);
}
} // namespace daml::api::client
