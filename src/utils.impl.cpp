module;

#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fmt/core.h>

module utils;

std::string get_env_var(std::string_view var_key)
{
    const std::string key(var_key);
    const char *env_var = std::getenv(key.c_str());

    if (env_var != nullptr)
    {
        return env_var;
    }
    throw std::runtime_error(fmt::format("Env variable not set: {}.", var_key));
}
