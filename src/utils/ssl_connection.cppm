module;

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <cpr/cpr.h>
#include <nlohmann/json_fwd.hpp>

export module daml.utils:ssl_connection;

export namespace daml::utils::ssl_connection
{
class SslConnection
{
  public:
    SslConnection(std::string_view url, std::optional<std::chrono::milliseconds> timeout);

    friend SslConnection &operator/(SslConnection &conn, std::string_view path);

    [[nodiscard]] std::string target();

    [[nodiscard]] cpr::Session &getSession();

  protected:
    cpr::Session session;
    std::string base_url;
};
} // namespace daml::api::ssl_connection
