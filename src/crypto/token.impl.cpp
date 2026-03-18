module;

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

module daml.crypto;

import daml.utils;

namespace daml::crypto::token
{
using namespace daml::utils;

std::string base64urlDecode(const std::string &input)
{
    std::string temp = input;

    // Replace URL-safe characters
    for (auto &c : temp)
    {
        if (c == '-')
            c = '+';
        else if (c == '_')
            c = '/';
    }

    // Add padding '='
    while (temp.size() % 4 != 0)
    {
        temp += '=';
    }

    // Standard Base64 decoding logic
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
        T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

    std::string output;
    int val = 0, valb = -8;
    for (unsigned char c : temp)
    {
        if (T[c] == -1)
            break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            output.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return output;
}

void TokenManager::load_token()
{
    if (!m_secrets.token.empty()) {
      m_token_data.m_token = m_secrets.token;
      m_token_data.decoded_json = decode_jwt_payload(m_token_data.m_token);
      return;
    }
    spdlog::info("[TokenManager] loadToken from server, MUST NOT BE EXECUTED OFTEN");

    ssl_connection::SslConnection connection(m_secrets.base_url, m_timeout);
    connection / "oauth" / "token";

    auto &session = connection.getSession();

    session.SetPayload(cpr::Payload{{"grant_type", "client_credentials"},
                                    {"client_id", m_secrets.client_id},
                                    {"client_secret", m_secrets.client_secret},
                                    {"audience", m_secrets.audience}});

    cpr::Response r = session.Post();

    if (r.status_code != 200)
    {
        throw std::runtime_error(
            std::format("[loadToken] request failed with status: {}, body: {}", r.status_code, r.text));
    }

    // parse response
    /*
    {
        "access_token": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...",
        "token_type": "Bearer",
        "expires_in": 86400
    }
    */
    auto json_resp = nlohmann::json::parse(r.text);

    m_token_data.m_token = json_resp.at("access_token").get<std::string>();
    m_token_data.decoded_json = decode_jwt_payload(m_token_data.m_token);
}

nlohmann::json TokenManager::decode_jwt_payload(const std::string &jwt)
{
    const auto first_dot = jwt.find('.');
    const auto second_dot = jwt.find('.', first_dot + 1);

    if (first_dot == std::string::npos || second_dot == std::string::npos)
        throw std::runtime_error("Invalid JWT format");

    const std::string payload_b64 = jwt.substr(first_dot + 1, second_dot - first_dot - 1);
    const std::string payload_json = base64urlDecode(payload_b64);

    nlohmann::json jv = nlohmann::json::parse(payload_json);
    if (!jv.is_object())
        throw std::runtime_error("JWT payload is not a JSON object");

    return jv;
}

std::string TokenManager::encrypt(std::string data, std::string_view key)
{
    for (size_t i = 0; i < data.size(); ++i)
        data[i] ^= key[i % key.size()];
    return data;
}

std::string TokenManager::decrypt(std::string data, std::string_view key)
{
    // XOR is symmetric
    return encrypt(std::move(data), key);
}

void TokenManager::f_save() const
{
    std::string blob = m_token_data.m_token;
    blob = encrypt(blob, m_secrets.key);

    const auto filePath = std::filesystem::current_path() / "data.cache";
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    file.write(blob.data(), blob.size());
}

void TokenManager::f_load()
{
    const auto filePath = std::filesystem::current_path() / "data.cache";
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filePath.string());

    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string blob(size, '\0');
    file.read(blob.data(), size);
    if (!file)
        throw std::runtime_error("Failed to read file: " + filePath.string());

    std::string decrypted = decrypt(blob, m_secrets.key);
    try
    {
        m_token_data.m_token = decrypted;
        m_token_data.decoded_json = decode_jwt_payload(decrypted);
    }
    catch (...)
    {
        m_token_data.m_token = blob;
        m_token_data.decoded_json = decode_jwt_payload(blob);
    }
}

std::string get_token()
{
    std::lock_guard lock(token_manager_mutex);
    if (!token_manager_inst)
        throw std::runtime_error("token manager is null");

    token_manager_inst->update_lazy();
    return token_manager_inst->get_token();
}
std::string get_user_id()
{
    std::lock_guard lock(token_manager_mutex);
    if (!token_manager_inst)
        throw std::runtime_error("token manager is null");

    token_manager_inst->update_lazy();
    return token_manager_inst->get_user_id();
}
} // namespace daml::crypto::token
