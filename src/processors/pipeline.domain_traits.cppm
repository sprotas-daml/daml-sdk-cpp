module;

#include <algorithm>
#include <string_view>
#include <vector>

export module pipeline.domain_traits;

import database;
import database.hold;

export namespace pipeline::traits
{
template <typename... Ts> struct TypeList
{
};
using RegisteredDomains = TypeList<db::hold::Update>;

template <typename T>
bool belongs_to_domain(std::string_view template_id, const std::vector<std::string> &interfaces_id = {});

template <>
bool belongs_to_domain<db::hold::Update>(std::string_view template_id, const std::vector<std::string> &interfaces_id)
{
    return std::ranges::any_of(interfaces_id, [](const auto &interface_id) {
        return interface_id.ends_with("Splice.Api.Token.HoldingV1:Holding");
    });
}

template <typename T> constexpr std::string_view domain_name_v = "UnknownDomain";

template <> constexpr std::string_view domain_name_v<db::hold::Update> = "HOLD";

} // namespace pipeline::traits
