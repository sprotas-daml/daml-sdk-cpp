module;

#include <algorithm>
#include <tuple>
#include <utility>

export module pipeline.router;

import manager;
import pipeline.domain_traits;

export namespace pipeline::router
{
template <typename T> struct DomainRawUpdate
{
    manager::ParsedUpdate data;
};

template <typename... Ts> auto split_update(manager::ParsedUpdate &&src, traits::TypeList<Ts...>)
{
    std::tuple<DomainRawUpdate<Ts>...> result;

    ((std::get<DomainRawUpdate<Ts>>(result).data.update_info = src.update_info), ...);

    auto route_items = [&result]<typename TCollection, typename TMember>(TCollection &source_range,
                                                                         TMember target_field) {
        std::ranges::for_each(source_range, [&result, target_field](auto &item) {
            [[maybe_unused]] bool r =
                ((traits::belongs_to_domain<Ts>(item.template_id, item.interface_ids) &&
                  ((std::get<DomainRawUpdate<Ts>>(result).data.*target_field).emplace_back(std::move(item)), true)) ||
                 ...);
        });
    };

    route_items(src.created_contracts, &manager::ParsedUpdate::created_contracts);
    route_items(src.choices, &manager::ParsedUpdate::choices);
    route_items(src.archived_contracts, &manager::ParsedUpdate::archived_contracts);

    return result;
}

bool has_changes(const manager::ParsedUpdate &u);
} // namespace pipeline::router