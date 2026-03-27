module;
#include <chrono>
#include <cstdint>
#include <format>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>

export module daml.api:utils;

import :client;
import :scan;
import :ledger;
import :submit;

import daml.utils;
import daml.model;
import daml.type;

export namespace daml::api::request
{
using int_t = std::uint64_t;
using str_ref_t = const std::string &;
using vec_str_ref_t = const std::vector<std::string> &;
using json = nlohmann::json;

using namespace daml::utils::json_literals;
using namespace daml::model::request;
using namespace daml::model::response;
using namespace daml::model::datatype;
using namespace daml::model::choice;

std::optional<DisclosedContract> get_featured_app_right_cid_for_provider(str_ref_t token, str_ref_t provider_party)
{
    auto featured_app_right_payload =
        client::scan_get(std::format("api/scan/v0/featured-apps/{}", provider_party)).at("featured_app_right");
    if (featured_app_right_payload.is_null())
        return std::nullopt;
    auto global_sync = get_global_sync(token);

    return DisclosedContract{
        .templateId = featured_app_right_payload.at("template_id"),
        .contractId = featured_app_right_payload.at("contract_id"),
        .createdEventBlob = featured_app_right_payload.at("created_event_blob"),
        .synchronizerId = global_sync,
    };
}

Prepared<PaymentTransferContext> get_payment_transfer_context(str_ref_t token, str_ref_t provider_party)
{
    TransferContext tf_ctx;
    std::vector<DisclosedContract> disclosed_contracts;

    auto featured_app_right = get_featured_app_right_cid_for_provider(token, provider_party);
    if (featured_app_right.has_value())
    {
        tf_ctx.featuredAppRight = featured_app_right.value().contractId;
        disclosed_contracts.emplace_back(std::move(featured_app_right.value()));
    }

    auto open_and_issuing_mining_rounds = get_active_open_and_issuing_rounds();
    DisclosedContract open_mining_round;
    node_int_t max_round_number = 0;
    for (const auto &round : open_and_issuing_mining_rounds.open_rounds)
    {
        if (round.round_number > max_round_number)
        {
            max_round_number = round.round_number;
            open_mining_round = round;
        }
    }
    tf_ctx.openMiningRound = open_mining_round.contractId;
    disclosed_contracts.emplace_back(std::move(open_mining_round));

    for (auto &issuing_round : open_and_issuing_mining_rounds.issuing_rounds)
    {
        IssuingMiningRound issuing_mining_round = {
            {
                .number = issuing_round.round_number,
            },
            issuing_round.contractId,
        };
        tf_ctx.issuingMiningRounds.emplace_back(std::move(issuing_mining_round));
        disclosed_contracts.emplace_back(std::move(issuing_round));
    }

    tf_ctx.validatorRights = {};

    PaymentTransferContext pay_tf_ctx;
    pay_tf_ctx.context = std::move(tf_ctx);

    auto amulet_rules = get_amulet_rules();
    pay_tf_ctx.amuletRules = amulet_rules.contractId;
    disclosed_contracts.emplace_back(std::move(amulet_rules));

    return {
        .data = std::move(pay_tf_ctx),
        .disclosed = std::move(disclosed_contracts),
    };
}

std::vector<ActiveContract> get_party_amulet_cids(str_ref_t token, str_ref_t party)
{
    return request::get_active_contract_set_by_template_with_parties(token, request::get_ledger_end(token),
                                                                     {"#splice-amulet:Splice.Amulet:Amulet"}, {party});
}

void create_transfer_preapproval_for_user(str_ref_t token, str_ref_t user_id, str_ref_t user_party,
                                          str_ref_t provider_party)
{
    auto pay_tf_ctx_prepared = get_payment_transfer_context(token, provider_party);
    auto disclosed_contracts = std::move(pay_tf_ctx_prepared.disclosed);
    auto pay_tf_ctx = std::move(pay_tf_ctx_prepared.data); // 1. context

    auto amulets = get_party_amulet_cids(token, provider_party);
    std::vector<TransferInput> transfer_inputs; // 2. inputs
    transfer_inputs.reserve(amulets.size());

    std::string expected_dso;

    for (const auto &amulet : amulets)
    {
        InputAmulet transfer_input;
        transfer_input.cid = amulet.createdEvent.contractId;
        expected_dso = amulet.createdEvent.createArgument.at("dso");
        transfer_inputs.emplace_back(transfer_input);
    }

    std::string amulet_rules_cid = pay_tf_ctx.amuletRules;
    std::string amulet_rules_tid;
    std::string syncronizer_id;

    for (const auto &discl : disclosed_contracts)
    {
        if (discl.contractId == amulet_rules_cid)
        {
            amulet_rules_tid = discl.templateId;
            syncronizer_id = discl.synchronizerId;
            break;
        }
    }

    auto now_time = std::chrono::system_clock::now();
    auto expires_at_time = now_time + std::chrono::hours(24 * 30);
    std::string expires_at = std::format("{:%Y-%m-%dT%H:%M:%S}Z", expires_at_time);

    const TransactionPayload tx = {
        .user_id = user_id,
        .read_as = {provider_party, user_party},
        .act_as = {provider_party, user_party},
        .synchronizer_id = syncronizer_id,
        .disclosed_contracts = disclosed_contracts,
        .commands = {ExerciseCommand{
            .choice = AmuletRules_CreateTransferPreapproval::choice_name,
            .contractId = amulet_rules_cid,
            .templateId = amulet_rules_tid,
            .choiceArgument =
                AmuletRules_CreateTransferPreapproval{
                    .context = pay_tf_ctx,
                    .inputs = transfer_inputs,
                    .receiver = user_party,
                    .provider = provider_party,
                    .expiresAt = expires_at,
                    .expectedDso = expected_dso,
                },
        }},
    };

    send_transaction(token, tx);
}

} // namespace daml::api::request
