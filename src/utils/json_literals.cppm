module;

#include <string>
#include <utility>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

export module daml.utils:json_literals;

namespace daml::utils::json_literals
{

export using json = nlohmann::json;

/* ---------------- pair builder ---------------- */

struct pair_builder
{
    std::string key;

    template <typename T> json operator=(T &&value) const
    {
        return json{{key, std::forward<T>(value)}};
    }

    json operator=(std::initializer_list<json> values) const
    {
        if (values.size() == 1)
            return json{{key, *values.begin()}};

        return json{{key, json(values)}};
    }

    // Accept string or const char*
    explicit pair_builder(std::string k) : key(std::move(k))
    {
    }
    explicit pair_builder(const char *k) : key(k)
    {
    }

    // Delete copy/move if you want builders to be non-copyable
    pair_builder(const pair_builder &) = delete;
    pair_builder(pair_builder &&) = delete;
};

/* ---------------- object builder ---------------- */

struct object_builder
{
    std::string key;

    json operator=(std::initializer_list<json> values) const
    {
        json obj = json::object();

        for (auto &v : values)
        {
            if (!v.is_object())
                throw std::runtime_error("json_object expects json objects");

            for (auto &[k, val] : v.items())
                obj[k] = val;
        }

        return json{{key, obj}};
    }

    explicit object_builder(std::string k) : key(std::move(k))
    {
    }
    explicit object_builder(const char *k) : key(k)
    {
    }

    object_builder(const object_builder &) = delete;
    object_builder(object_builder &&) = delete;
};

/* ---------------- array builder ---------------- */

struct array_builder
{
    std::string key;

    json operator=(std::initializer_list<json> values) const
    {
        json arr = json::array();

        for (auto &v : values)
            arr.push_back(v);

        return json{{key, arr}};
    }

    json operator[](const json &arg0) const
    {
        return json{{key, {arg0}}};
    }

    explicit array_builder(std::string k) : key(std::move(k))
    {
    }
    explicit array_builder(const char *k) : key(k)
    {
    }

    array_builder(const array_builder &) = delete;
    array_builder(array_builder &&) = delete;
};

/* ---------------- anonymous helpers ---------------- */

export struct jo
{
    json value;

    jo(std::initializer_list<json> values)
    {
        value = json::object();

        for (auto &v : values)
        {
            if (!v.is_object())
                throw std::runtime_error("jo{} expects json objects");

            for (auto &[k, val] : v.items())
                value[k] = val;
        }
    }

    operator json() const
    {
        return value;
    }
};

export struct ja
{
    json value;

    ja(std::initializer_list<json> values)
    {
        value = json::array();
        for (auto &v : values)
            value.push_back(v);
    }

    operator json() const
    {
        return value;
    }
};

/* ---------------- runtime key helper ---------------- */

export inline pair_builder key(std::string k)
{
    return pair_builder{std::move(k)};
}

/* ---------------- literals ---------------- */

export inline pair_builder operator""_j(const char *str, std::size_t)
{
    return pair_builder{str};
}

export inline array_builder operator""_ja(const char *str, std::size_t)
{
    return array_builder{str};
}

export inline object_builder operator""_jo(const char *str, std::size_t)
{
    return object_builder{str};
}

} // namespace daml::utils::json_literals
