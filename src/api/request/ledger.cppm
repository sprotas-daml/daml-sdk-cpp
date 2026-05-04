module;
#include <cstdint>
#include <format>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>

export module daml.api:ledger;

import :client;

import daml.utils;
import daml.model;
import daml.type;

export namespace daml::api::request
{
using str_ref_t = const std::string &;
using vec_str_ref_t = const std::vector<std::string> &;
using json = nlohmann::json;

using namespace daml::utils::json_literals;
using namespace daml::model::request;
using namespace daml::model::response;

enum TransactionShape
{
    Unspecified,
    AcsDelta,
    LedgerEffects,
};

const std::unordered_map<TransactionShape, std::string> transaction_shape_map = {
    {TransactionShape::Unspecified, "TRANSACTION_SHAPE_UNSPECIFIED"},
    {TransactionShape::AcsDelta, "TRANSACTION_SHAPE_ACS_DELTA"},
    {TransactionShape::LedgerEffects, "TRANSACTION_SHAPE_LEDGER_EFFECTS"},
};

bool check_and_grant_rights(str_ref_t token, str_ref_t user_id)
{
    auto rights = client::ledger_get("v2/users/" + user_id + "/rights", token);

    if (rights.is_discarded() or !rights.is_object() or !rights.contains("rights") or !rights["rights"].is_array())
    {
        return false;
    }

    for (const auto &item : rights["rights"])
    {
        if (item.is_object() and item.contains("kind") and item["kind"].is_object())
        {
            if (item["kind"].contains("CanReadAsAnyParty"))
            {
                return true;
            }
        }
    }

    auto new_rights = jo{
        "userId"_j = user_id,
        "rights"_ja = {jo{"kind"_jo = {"CanReadAsAnyParty"_jo = {"value"_jo = {}}}}},
    };

    client::ledger_post("v2/users/" + user_id + "/rights", token, new_rights);
    return true;
}

node_int_t get_ledger_end(str_ref_t token)
{
    return client::ledger_get("/v2/state/ledger-end", token).at("offset").get<node_int_t>();
}

json get_filters(vec_str_ref_t parties, vec_str_ref_t template_ids, vec_str_ref_t interface_ids, bool use_wildcard = false)
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

    if (use_wildcard || filters.cumulative.empty())
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

json get_event_format(vec_str_ref_t parties, vec_str_ref_t template_ids, vec_str_ref_t interface_ids, bool verbose, bool use_wildcard = false)
{
    auto j = get_filters(parties, template_ids, interface_ids, use_wildcard);
    j["verbose"] = verbose;
    return j;
}

std::vector<Update> get_updates_flats(str_ref_t token, node_int_t from, node_int_t to, vec_str_ref_t template_ids,
                                      vec_str_ref_t interface_ids, vec_str_ref_t parties,
                                      TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    UpdatesRequest req = {
        .beginExclusive = from,
        .endInclusive = to,
        .updateFormat =
            {
                .includeTransactions =
                    {
                        .transactionShape = transaction_shape_map.at(transaction_shape),
                        .eventFormat = get_event_format(parties, template_ids, interface_ids, true),
                    },
            },
    };

    return client::ledger_post("v2/updates/flats", token, req);
}

std::vector<Update> get_updates_flats_by_template(str_ref_t token, node_int_t from, node_int_t to,
                                                  vec_str_ref_t template_ids,
                                                  TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    return get_updates_flats(token, from, to, template_ids, {}, {}, transaction_shape);
}

std::vector<Update> get_updates_flats_by_template_and_interface(
    str_ref_t token, node_int_t from, node_int_t to, vec_str_ref_t template_ids, vec_str_ref_t interface_ids,
    TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    return get_updates_flats(token, from, to, template_ids, interface_ids, {}, transaction_shape);
}

std::vector<Update> get_updates_flats_by_template_and_interface_with_parties(
    str_ref_t token, node_int_t from, node_int_t to, vec_str_ref_t template_ids, vec_str_ref_t interface_ids,
    vec_str_ref_t parties, TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    return get_updates_flats(token, from, to, template_ids, interface_ids, parties, transaction_shape);
}
std::vector<Update> get_updates_flats_by_interface(str_ref_t token, node_int_t from, node_int_t to,
                                                   vec_str_ref_t interface_ids,
                                                   TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    return get_updates_flats(token, from, to, {}, interface_ids, {}, transaction_shape);
}
std::vector<Update> get_updates_flats_by_template_with_parties(
    str_ref_t token, node_int_t from, node_int_t to, vec_str_ref_t template_ids, vec_str_ref_t parties,
    TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    return get_updates_flats(token, from, to, template_ids, {}, parties, transaction_shape);
}
std::vector<Update> get_updates_flats_by_interface_with_parties(
    str_ref_t token, node_int_t from, node_int_t to, vec_str_ref_t interface_ids, vec_str_ref_t parties,
    TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    return get_updates_flats(token, from, to, {}, interface_ids, parties, transaction_shape);
}

std::vector<ActiveContract> get_active_contract_set(str_ref_t token, node_int_t offset, vec_str_ref_t template_ids,
                                                    vec_str_ref_t interface_ids, vec_str_ref_t parties)
{
    ActiveContractSetRequest req = {
        .activeAtOffset = offset,
        .verbose = false,
        .filter = get_filters(parties, template_ids, interface_ids),
    };

    return client::ledger_post("v2/state/active-contracts", token, req, {{"limit", "200"}});
}

std::vector<ActiveContract> get_active_contract_set_by_template(str_ref_t token, node_int_t offset,
                                                                vec_str_ref_t template_ids)
{
    return get_active_contract_set(token, offset, template_ids, {}, {});
}
std::vector<ActiveContract> get_active_contract_set_by_interface(str_ref_t token, node_int_t offset,
                                                                 vec_str_ref_t interface_ids)
{
    return get_active_contract_set(token, offset, {}, interface_ids, {});
}
std::vector<ActiveContract> get_active_contract_set_by_template_with_parties(str_ref_t token, node_int_t offset,
                                                                             vec_str_ref_t template_ids,
                                                                             vec_str_ref_t parties)
{
    return get_active_contract_set(token, offset, template_ids, {}, parties);
}
std::vector<ActiveContract> get_active_contract_set_by_interface_with_parties(str_ref_t token, node_int_t offset,
                                                                              vec_str_ref_t interface_ids,
                                                                              vec_str_ref_t parties)
{
    return get_active_contract_set(token, offset, {}, interface_ids, parties);
}

Update get_update_by_id_with_filters(str_ref_t token, str_ref_t update_id, vec_str_ref_t template_ids,
                                     vec_str_ref_t interface_ids, bool use_wildcard,
                                     TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    UpdateByIdRequest req = {
        .updateId = update_id,
        .updateFormat =
            {
                .includeTransactions =
                    {
                        .transactionShape = transaction_shape_map.at(transaction_shape),
                        .eventFormat = get_event_format({}, template_ids, interface_ids, true, use_wildcard),
                    },
            },
    };

    return client::ledger_post("v2/updates/update-by-id", token, req);
}

Update get_update_by_id_with_parties(str_ref_t token, str_ref_t update_id, vec_str_ref_t parties,
                                     TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    UpdateByIdRequest req = {
        .updateId = update_id,
        .updateFormat =
            {
                .includeTransactions =
                    {
                        .transactionShape = transaction_shape_map.at(transaction_shape),
                        .eventFormat = get_event_format(parties, {}, {}, true),
                    },
            },
    };

    return client::ledger_post("v2/updates/update-by-id", token, req);
}

Update get_update_by_id(str_ref_t token, str_ref_t update_id,
                        TransactionShape transaction_shape = TransactionShape::AcsDelta)
{
    return get_update_by_id_with_parties(token, update_id, {}, transaction_shape);
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

json get_parties(str_ref_t token, str_ref_t next_page_token = {}, node_int_t page_size = {})
{
    client::Params params;
    if (!next_page_token.empty())
    {
        params.emplace_back("pageToken", next_page_token);
    }
    if (page_size != node_int_t{})
    {
        params.emplace_back("pageSize", std::to_string(page_size));
    }
    return client::ledger_get("v2/parties", token, params);
}

std::string get_global_sync(str_ref_t token)
{
    auto synchronizers = client::ledger_get("v2/state/connected-synchronizers", token)
                             .at("connectedSynchronizers")
                             .get<std::vector<json>>();
    for (const auto &sync : synchronizers)
    {
        if (sync.value("synchronizerAlias", "") == "global")
        {
            return sync.value("synchronizerId", "");
        }
    }
    return {};
}

}; // namespace daml::api::request
