module;
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

export module daml.api:scan;

import :client;

import daml.utils;
import daml.model;
import daml.type;

export namespace daml::api::request
{
using int_t = int32_t;
using str_ref_t = const std::string &;
using json = nlohmann::json;

using namespace daml::utils::json_literals;
using namespace daml::model::datatype;
using namespace daml::model::request;
using namespace daml::model::response;

DisclosedContract get_amulet_rules(str_ref_t token)
{
    const nlohmann::json payload = {{"cached_amulet_rules_contract_id", ""}, {"cached_amulet_rules_domain_id", ""}};
    auto j = client::scan_post("v0/amulet-rules", payload);
    auto contract = j.at("amulet_rules_update").at("contract");
    auto sync = j.at("amulet_rules_update").at("domain_id");

    return DisclosedContract{
        .templateId = contract.at("template_id"),
        .contractId = contract.at("contract_id"),
        .createdEventBlob = contract.at("created_event_blob"),
        .synchronizerId = sync,
    };
}

DisclosedContract get_external_party_amulet_rules()
{
    const nlohmann::json payload = jo{
        "cached_external_party_amulet_rules_contract_id"_j = "",
        "cached_external_party_amulet_rules_domain_id"_j = "",
    };
    auto j = client::scan_post("v0/external-party-amulet-rules", payload);
    auto contract = j.at("amulet_rules_update").at("contract");
    auto sync = j.at("amulet_rules_update").at("domain_id");

    return DisclosedContract{
        .templateId = contract.at("template_id"),
        .contractId = contract.at("contract_id"),
        .createdEventBlob = contract.at("created_event_blob"),
        .synchronizerId = sync,
    };
}

OpenAndIssuingRounds get_open_and_issuing_rounds()
{
    const std::string endpoint = "v0/open-and-issuing-mining-rounds";
    const nlohmann::json payload = {{"cached_open_mining_round_contract_ids", nlohmann::json::array()},
                                    {"cached_issuing_round_contract_ids", nlohmann::json::array()}};
    auto response = client::scan_post(endpoint, payload);

    const auto issuing_json = response.at("issuing_mining_rounds");
    const auto open_json = response.at("open_mining_rounds");

    std::vector<Round> issuing_rounds;
    for (const auto &[key, value] : issuing_json.items())
    {
        const auto &contract = value.at("contract");
        const std::string opens_at = contract.at("payload").at("opensAt").get<std::string>();
        const std::string closes_at = contract.at("payload").at("targetClosesAt").get<std::string>();

        Round round;
        round.templateId = contract.at("template_id").get<std::string>();
        round.contractId = contract.at("contractId").get<std::string>();
        round.createdEventBlob = contract.at("createdEventBlob").get<std::string>();
        round.synchronizerId = value.at("domain_id").get<std::string>();
        round.round_number = std::stoi(contract.at("payload").at("round").at("number").get<std::string>());
        round.opens_at = utils::time::parse_utc_iso8601(opens_at);
        round.target_closes_at = utils::time::parse_utc_iso8601(closes_at);
        round.issuing_round_coefficient =
            contract.at("payload").at("issuancePerFeaturedAppRewardCoupon").get<decimal::decimal>();
        issuing_rounds.emplace_back(std::move(round));
    }
    std::vector<Round> open_rounds;
    for (const auto &[key, value] : open_json.items())
    {
        const auto &contract = value.at("contract");
        const std::string opens_at = contract.at("payload").at("opensAt").get<std::string>();
        const std::string closes_at = contract.at("payload").at("targetClosesAt").get<std::string>();

        Round round;
        round.templateId = contract.at("template_id").get<std::string>();
        round.contractId = contract.at("contractId").get<std::string>();
        round.createdEventBlob = contract.at("createdEventBlob").get<std::string>();
        round.synchronizerId = value.at("domain_id").get<std::string>();
        round.round_number = std::stoi(contract.at("payload").at("round").at("number").get<std::string>());
        round.opens_at = utils::time::parse_utc_iso8601(opens_at);
        round.target_closes_at = utils::time::parse_utc_iso8601(closes_at);
        open_rounds.emplace_back(std::move(round));
    }
    return {
        .open_rounds = std::move(open_rounds),
        .issuing_rounds = std::move(issuing_rounds),
    };
}

OpenAndIssuingRounds get_active_open_and_issuing_rounds()
{
    using namespace std::chrono;

    const auto now = floor<microseconds>(system_clock::now());

    auto is_inactive = [now](const Round &round) -> bool {
        return !(round.opens_at <= now && now < round.target_closes_at);
    };

    auto rounds = get_open_and_issuing_rounds();

    std::erase_if(rounds.open_rounds, is_inactive);
    std::erase_if(rounds.issuing_rounds, is_inactive);
    return rounds;
}

std::vector<Round> get_open_rounds()
{
    return get_open_and_issuing_rounds().open_rounds;
}

std::vector<Round> get_active_open_rounds()
{
    return get_active_open_and_issuing_rounds().open_rounds;
}

std::vector<Round> get_issuing_rounds()
{
    return get_open_and_issuing_rounds().issuing_rounds;
}

std::vector<Round> get_active_issuing_rounds()
{
    return get_active_open_and_issuing_rounds().issuing_rounds;
}

}; // namespace daml::api::request
