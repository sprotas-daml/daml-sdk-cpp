module;

#include <chrono>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <string_view>

module daml.api;

namespace daml::api::ssl_connection
{
SslConnection::SslConnection(const std::string_view url, const std::optional<std::chrono::milliseconds> timeout)
    : base_url(url)
{
    if (base_url.empty())
    {
        throw std::runtime_error("url is empty");
    }

    session.SetUrl(cpr::Url{base_url});

    if (timeout)
    {
        session.SetTimeout(cpr::Timeout{*timeout});
    }

    session.SetVerifySsl(cpr::VerifySsl{true});

    session.SetSslOptions(cpr::Ssl(cpr::ssl::TLSv1_2{}));
}

SslConnection &operator/(SslConnection &conn, std::string_view path)
{
    if (path.empty())
        return conn;

    std::string current_url = conn.session.GetFullRequestUrl();

    bool base_has_slash = (!current_url.empty() && current_url.back() == '/');
    bool path_has_slash = (path.front() == '/');

    if (base_has_slash && path_has_slash)
    {
        path.remove_prefix(1);
        current_url.append(path);
    }
    else if (!base_has_slash && !path_has_slash)
    {
        current_url += '/';
        current_url.append(path);
    }
    else
    {
        current_url.append(path);
    }

    conn.session.SetUrl(cpr::Url{current_url});
    return conn;
}

std::string SslConnection::target()
{
    return session.GetFullRequestUrl();
}

cpr::Session &SslConnection::getSession()
{
    return session;
}
} // namespace daml::api::ssl_connection
