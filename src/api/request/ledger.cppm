module;
#include <cstdint>
#include <format>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

export module daml.api:ledger;

import :client;

import daml.utils;
import daml.model;

export namespace daml::api::request
{
using int_t = int32_t;
using str_ref_t = const std::string &;
using vec_str_ref_t = const std::vector<std::string> &;
using json = nlohmann::json;

using namespace daml::utils::json_literals;
using namespace daml::model::request;
using namespace daml::model::response;

int_t get_ledger_end(str_ref_t token)
{
    int64_t offset_value = client::ledger_get("/v2/state/ledger-end", token).at("offset").get<int64_t>();
    if (offset_value < std::numeric_limits<int_t>::min() || offset_value > std::numeric_limits<int_t>::max())
        throw std::runtime_error(std::format("[get_ledger_end] offset out of int32 range: {}", offset_value));

    return static_cast<int_t>(offset_value);
}

json get_filters(vec_str_ref_t parties, vec_str_ref_t template_ids, vec_str_ref_t interface_ids)
{
    CumulativeFilters filters;
    for (const auto &interface_id : interface_ids)
    {
        filters.cumulative.emplace_back(FilterPayload{
            .identifierFilter =
                InterfaceFilter{
                    .value =
                        {
                            .interfaceId = interface_id,
                            .includeCreatedEventBlob = true,
                            .includeInterfaceView = true,
                        },
                },
        });
    }

    for (const auto &template_id : template_ids)
    {
        filters.cumulative.emplace_back(FilterPayload{
            .identifierFilter =
                TemplateFilter{
                    .value =
                        {
                            .templateId = template_id,
                            .includeCreatedEventBlob = true,
                        },
                },
        });
    }

    if (filters.cumulative.empty())
    {
        filters.cumulative.emplace_back(FilterPayload{
            .identifierFilter =
                WildcardFilter{
                    .value =
                        {
                            .includeCreatedEventBlob = true,
                        },
                },
        });
    }
    if (parties.empty())
    {
        return jo{
            "filtersForAnyParty"_j = filters,
        };
    }

    json parties_object = json::object();
    for (const auto &party : parties)
    {
        parties_object[party] = filters;
    }

    return jo{
        "filtersByParty"_j = parties_object,
    };
}

json get_event_format(vec_str_ref_t parties, vec_str_ref_t template_ids, vec_str_ref_t interface_ids, bool verbose)
{
    auto j = get_filters(parties, template_ids, interface_ids);
    j["verbose"] = verbose;
    return j;
}

std::vector<Update> get_updates_flats(str_ref_t token, int_t from, int_t to, vec_str_ref_t template_ids,
                                      vec_str_ref_t interface_ids, vec_str_ref_t parties)
{
    FlatUpdatesRequest req;
    req.beginExclusive = from, req.endInclusive = to, req.verbose = false,
    req.filter = get_filters(parties, template_ids, interface_ids);

    return client::ledger_post("v2/updates/flats", token, req);
}

std::vector<Update> get_updates_flats_by_template(str_ref_t token, int_t from, int_t to, vec_str_ref_t template_ids)
{
    return get_updates_flats(token, from, to, template_ids, {}, {});
}
std::vector<Update> get_updates_flats_by_interface(str_ref_t token, int_t from, int_t to, vec_str_ref_t interface_ids)
{
    return get_updates_flats(token, from, to, {}, interface_ids, {});
}
std::vector<Update> get_updates_flats_by_template_with_parties(str_ref_t token, int_t from, int_t to,
                                                               vec_str_ref_t template_ids, vec_str_ref_t parties)
{
    return get_updates_flats(token, from, to, template_ids, {}, parties);
}
std::vector<Update> get_updates_flats_by_interface_with_parties(str_ref_t token, int_t from, int_t to,
                                                                vec_str_ref_t interface_ids, vec_str_ref_t parties)
{
    return get_updates_flats(token, from, to, {}, interface_ids, parties);
}

std::vector<ActiveContract> get_active_contract_set(str_ref_t token, int_t offset, vec_str_ref_t template_ids,
                                                    vec_str_ref_t interface_ids, vec_str_ref_t parties)
{
    ActiveContractSetRequest req;
    req.activeAtOffset = offset;
    req.verbose = false;
    req.filter = get_filters(parties, template_ids, interface_ids);

    return client::ledger_post("v2/state/active-contracts", token, req);
}

std::vector<ActiveContract> get_active_contract_set_by_template(str_ref_t token, int_t offset,
                                                                std::vector<std::string> &template_ids)
{
    return get_active_contract_set(token, offset, template_ids, {}, {});
}
std::vector<ActiveContract> get_active_contract_set_by_interface(str_ref_t token, int_t offset,
                                                                 std::vector<std::string> &interface_ids)
{
    return get_active_contract_set(token, offset, {}, interface_ids, {});
}
std::vector<ActiveContract> get_active_contract_set_by_template_with_parties(str_ref_t token, int_t offset,
                                                                             vec_str_ref_t template_ids,
                                                                             vec_str_ref_t parties)
{
    return get_active_contract_set(token, offset, template_ids, {}, parties);
}
std::vector<ActiveContract> get_active_contract_set_by_interface_with_parties(str_ref_t token, int_t offset,
                                                                              vec_str_ref_t interface_ids,
                                                                              vec_str_ref_t parties)
{
    return get_active_contract_set(token, offset, {}, interface_ids, parties);
}

Update get_update_by_id_with_parties(str_ref_t token, str_ref_t update_id, vec_str_ref_t parties)
{
    UpdateByIdRequest req = {
        .updateId = update_id,
        .updateFormat =
            {
                .includeTransactions =
                    {
                        .transactionShape = "TRANSACTION_SHAPE_ACS_DELTA",
                        .eventFormat = get_event_format(parties, {}, {}, true),
                    },
            },
    };

    return client::ledger_post("v2/updates/update-by-id", token, req);
}

Update get_update_by_id(str_ref_t token, str_ref_t update_id)
{
    return get_update_by_id_with_parties(token, update_id, {});
};

ContractEvents get_contract_by_id_with_parties(str_ref_t token, str_ref_t contract_id, vec_str_ref_t parties)
{
    ContractByIdRequest req = {
        .contractId = contract_id,
        .eventFormat = get_event_format(parties, {}, {}, true),
    };

    return client::ledger_post("v2/events/events-by-contract-id", token, req);
}

ContractEvents get_contract_by_id(str_ref_t token, str_ref_t contract_id)
{
    return get_contract_by_id_with_parties(token, contract_id, {});
}

}; // namespace daml::api::request
