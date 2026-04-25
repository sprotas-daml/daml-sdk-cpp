module;

#include <string>
#include <string_view>

export module utils;

export std::string get_env_var(std::string_view var_key);
