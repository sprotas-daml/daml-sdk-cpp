module;
#include <nlohmann/json.hpp>
#include <string>

export module daml.utils:error;

export namespace daml::util::error
{
std::string extract_daml_error(const std::string &input_json)
{
    try
    {
        auto jv = nlohmann::json::parse(input_json);

        std::string_view cause = jv.at("cause").get<std::string_view>();

        const std::string_view separator = "): ";
        size_t pos = cause.find(separator);

        if (pos != std::string_view::npos)
        {
            return std::string(cause.substr(pos + separator.length()));
        }

        size_t last_colon = cause.find_last_of(':');
        if (last_colon != std::string_view::npos)
        {
            std::string_view result = cause.substr(last_colon + 1);
            if (!result.empty() && result[0] == ' ')
            {
                result.remove_prefix(1);
            }
            return std::string(result);
        }

        return std::string(cause);
    }
    catch (const std::exception &e)
    {
        return "Parsing error: " + std::string(e.what());
    }
}
} // namespace daml::util::error
