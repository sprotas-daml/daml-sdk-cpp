
module;

#include <cpr/cpr.h>
#include <cpr/curl_container.h>
#include <cpr/parameters.h>
#include <format>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

export module daml.api:client;

import :registry;
import daml.utils;

export namespace daml::api::client
{
using registry::NodeConfig;
using registry::Registry;

using namespace daml::utils;

class daml_network_error : public std::runtime_error { // TODO: move to separate module and namespace
private:
    long m_status;
    std::string m_body;

public:
    daml_network_error(const std::string& message, int status, std::string body)
        : std::runtime_error(message), 
          m_status(status), 
          m_body(std::move(body)) {}

    [[nodiscard]] long status() const noexcept {
        return m_status;
    }

    [[nodiscard]] const std::string& body() const noexcept {
        return m_body;
    }
};

using Params = std::vector<std::pair<std::string, std::string>>;

nlohmann::json post(const NodeConfig &config, std::string_view path, const std::string &token,
                    const nlohmann::json &body, const Params &parameters = {})
{
    ssl_connection::SslConnection conn(config.url, config.timeout);
    conn / path;

    std::string target_url = conn.target();
    spdlog::debug("POST request to {}", target_url);

#ifdef SHOW_PAYLOAD
    spdlog::trace("Payload: {}", body.dump(2));
#endif
    cpr::Session &session = conn.getSession();

    if (!parameters.empty())
    {
        cpr::Parameters params;

        for (const auto &[key, value] : parameters)
        {
            params.Add(cpr::Parameter{key, value});
        }

        session.SetParameters(params);
    }

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
        throw daml_network_error(
            std::format("Post to {} failed with status {}: {}", target_url, response.status_code, response.text), response.status_code, response.text);
    }
#ifdef SHOW_PAYLOAD
    spdlog::trace("Response: {}", response.text);
#endif

    return nlohmann::json::parse(response.text);
}

nlohmann::json get(const NodeConfig &config, std::string_view path, const std::string &token,
                   const Params &parameters = {})
{
    ssl_connection::SslConnection conn(config.url, config.timeout);
    conn / path;

    std::string target_url = conn.target();
    spdlog::debug("GET request to {}", target_url);

    cpr::Session &session = conn.getSession();

    if (!parameters.empty())
    {
        cpr::Parameters params;

        for (const auto &[key, value] : parameters)
        {
            params.Add(cpr::Parameter{key, value});
        }

        session.SetParameters(params);
    }

    session.SetHeader(cpr::Header{{"Accept", "application/json"}, {"Authorization", std::format("Bearer {}", token)}});
    cpr::Response response = session.Get();

    if (response.error.code != cpr::ErrorCode::OK)
    {
        throw std::runtime_error(std::format("GET transport error to {}: {}", target_url, response.error.message));
    }
    
    spdlog::debug("Response: {}", response.text);

    if (response.status_code != 200)
    {
        throw std::runtime_error(
            std::format("GET to {} failed with status {}: {}", target_url, response.status_code, response.text));
    }

    return nlohmann::json::parse(response.text);
}

nlohmann::json ledger_get(std::string_view path, const std::string &token, const Params &params = {})
{
    return get(Registry::get_ledger(), path, token, params);
}
nlohmann::json scan_get(std::string_view path)
{
    return get(Registry::get_scan(), path, "");
}
nlohmann::json ledger_post(std::string_view path, const std::string &token, const nlohmann::json &body, const Params &params = {})
{
    return post(Registry::get_ledger(), path, token, body, params);
}
nlohmann::json scan_post(std::string_view path, const nlohmann::json &body)
{
    return post(Registry::get_scan(), path, "", body);
}
} // namespace daml::api::client
