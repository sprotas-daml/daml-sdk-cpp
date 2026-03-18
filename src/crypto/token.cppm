module;

#include <string>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <utility>

// curl -X POST 'https://some_id.us.auth0.com/oauth/token'
// --header 'Content-Type: application/x-www-form-urlencoded'
// --data 'grant_type=client_credentials'
// --data 'client_id=some_id'
// --data 'client_secret=some_secret'
// --data 'audience=some_ref'

export module daml.crypto:token;

export namespace daml::crypto::token
{
using namespace std::chrono_literals;

class TokenManager
{
  public:
    struct Secrets
    {
        std::string base_url;
        std::string client_id;
        std::string client_secret;
        std::string audience;
        std::string key;
        std::string token;
    };
    explicit TokenManager(Secrets &&secrets)
        : m_secrets(std::move(secrets)), last_lazy_update(std::chrono::system_clock::now() - 1h)
    {
        if (m_secrets.audience.empty() or m_secrets.client_id.empty() or m_secrets.client_secret.empty() or
            m_secrets.base_url.empty())
            throw std::invalid_argument("TokenManager::TokenManager()");
    }

    ~TokenManager()
    {
        std::ranges::fill(m_secrets.base_url, '\0');
        std::ranges::fill(m_secrets.client_secret, '\0');
        std::ranges::fill(m_secrets.client_id, '\0');
        std::ranges::fill(m_secrets.audience, '\0');
        std::ranges::fill(m_secrets.key, '\0');
    }

    void update_lazy()
    {
        try
        {
            f_load();
        }
        catch (...)
        {
        }

        if (std::chrono::system_clock::now() < last_lazy_update + 1min)
        {
            spdlog::debug("[TokenManager] Skip token update: Too much token updates (max is one per minute)");
            return;
        }
        if (!m_token_data.m_token.empty() and std::chrono::system_clock::now() + 30s < getExpiresAt())
        {
            spdlog::debug("[TokenManager::update_lazy()] Skip token update: Token is still valid");
            return;
        }

        spdlog::info("[TokenManager::update_lazy()] checks passed");
        last_lazy_update = std::chrono::system_clock::now();
        load_token();
        f_save();
        f_load();
    }

    [[nodiscard]]
    std::chrono::time_point<std::chrono::system_clock> getExpiresAt() const
    {
        const std::chrono::seconds secs{m_token_data.decoded_json.at("exp").get<int64_t>()};
        return std::chrono::system_clock::time_point{secs};
    }

    [[nodiscard]]
    std::string get_user_id() const
    {
        return m_token_data.decoded_json.at("sub").get<std::string>();
    }

    [[nodiscard]]
    std::string get_token() const
    {
        return m_token_data.m_token;
    }

  private:
    Secrets m_secrets;
    std::chrono::milliseconds m_timeout = 1min;

    struct TokenData
    {
        std::string m_token;
        nlohmann::json decoded_json;
    } m_token_data;

    std::chrono::system_clock::time_point last_lazy_update;

    void load_token();

    static nlohmann::json decode_jwt_payload(const std::string &jwt);

    static std::string encrypt(std::string data, std::string_view key);

    static std::string decrypt(std::string data, std::string_view key);

    void f_save() const;

    void f_load();
};

std::mutex token_manager_mutex;
std::optional<TokenManager> token_manager_inst;

std::string get_token();
std::string get_user_id();

} // namespace daml::crypto::token
