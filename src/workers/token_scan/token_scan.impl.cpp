module;
#include <algorithm>
#include <chrono>
#include <cpr/cpr.h>
#include <cpr/curl_container.h>
#include <cpr/parameters.h>
#include <format>
#include <memory>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <thread>

module token_scan;

import daml.utils;
import database.token;

namespace token_scan
{

struct InstrumentContract
{
    std::optional<std::string> createdAt;
    std::string contractId;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InstrumentContract, createdAt, contractId)

struct DefaultInstrumentIdentifier
{
    std::string source;
    std::string id;
    std::string scheme;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DefaultInstrumentIdentifier, source, id, scheme)

struct InstrumentPayload
{
    std::string instr_operator;
    std::string provider;
    std::string registrar;
    DefaultInstrumentIdentifier defaultIdentifier;
};

void to_json(nlohmann::json &j, const InstrumentPayload &p)
{
    j = nlohmann::json{{"operator", p.instr_operator},
                       {"provider", p.provider},
                       {"registrar", p.registrar},
                       {"defaultIdentifier", p.defaultIdentifier}};
}

void from_json(const nlohmann::json &j, InstrumentPayload &p)
{
    p.instr_operator = j.value("operator", "");
    p.provider = j.value("provider", "");
    p.registrar = j.value("registrar", "");
    p.defaultIdentifier = j.at("defaultIdentifier");
}

struct InstrumentConfiguration
{
    InstrumentContract contract;
    InstrumentPayload payload;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InstrumentConfiguration, contract, payload)

struct Instrument
{
    std::string id;
    std::string name;
    std::string symbol;
    std::optional<std::string> totalSupply;
    std::int32_t decimals;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Instrument, id, name, symbol, totalSupply, decimals)

struct InstrumentsPage
{
    std::vector<Instrument> instruments;
    std::optional<std::string> nextPageToken;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InstrumentsPage, instruments, nextPageToken)

TokenScanner::TokenScanner(config::ScanConfig cfg, std::shared_ptr<db::Connection> m_db) : m_cfg(cfg), m_db(m_db)
{
}

using ParsedInstrumentsMap = std::unordered_map<std::string, std::unordered_map<std::string, Instrument>>;

void update_instruments_map_for_registrar(const std::string &registrar, ParsedInstrumentsMap &instruments,
                                          daml::utils::ssl_connection::SslConnection &token_standard, size_t page_size)
{
    std::string target_url = token_standard.target();

    cpr::Session &token_standard_session = token_standard.getSession();

    token_standard_session.SetHeader(cpr::Header{{"Accept", "application/json"}});

    std::string page_token = "";
    do
    {
        token_standard_session.SetParameters(cpr::Parameters{
            cpr::Parameter{"pageSize", std::to_string(page_size)},
            cpr::Parameter{"pageToken", page_token},
        });

        cpr::Response response;
        int delay_ms = 1000;

        while (true)
        {
            response = token_standard_session.Get();
            if (response.error.code != cpr::ErrorCode::OK)
            {
                throw std::runtime_error(
                    std::format("GET transport error to {}: {}", target_url, response.error.message));
            }

            if (response.status_code == 429)
            {
                spdlog::warn(std::format("GET to {} returned 429. Retrying in {} ms...", target_url, delay_ms));

                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

                delay_ms = std::min(delay_ms * 2, 30000);

                continue;
            }

            if (response.status_code != 200)
            {
                throw std::runtime_error(std::format("GET to {} failed with status {}: {}", target_url,
                                                     response.status_code, response.text));
            }
            break;
        }

        InstrumentsPage page = nlohmann::json::parse(response.text);
        page_token = page.nextPageToken.value_or("");

        for (auto &instrument : page.instruments)
        {
            spdlog::debug(std::format("{}::{}", instrument.id, registrar));
            instruments[registrar][instrument.id] = instrument;
        }

    } while (!page_token.empty());
}

void TokenScanner::run_once()
{
    using namespace daml::utils::ssl_connection;
    using namespace db::token;
    auto instrument_configuration = SslConnection(m_cfg.utility_base_url, m_cfg.request_timeout);
    instrument_configuration / "utilities" / "v0" / "contract" / "instrument-configuration" / "all";
    std::string target_url = instrument_configuration.target();

    cpr::Session &instr_conf_session = instrument_configuration.getSession();

    instr_conf_session.SetHeader(cpr::Header{{"Accept", "application/json"}});
    cpr::Response response = instr_conf_session.Get();
    if (response.error.code != cpr::ErrorCode::OK)
    {
        throw std::runtime_error(std::format("GET transport error to {}: {}", target_url, response.error.message));
    }

    if (response.status_code != 200)
    {
        throw std::runtime_error(
            std::format("GET to {} failed with status {}: {}", target_url, response.status_code, response.text));
    }

    std::vector<InstrumentConfiguration> configs = nlohmann::json::parse(response.text).at("instrumentConfigurations");

    ParsedInstrumentsMap instruments;
    std::vector<TokenInstrument> parsed_instruments;
    parsed_instruments.reserve(configs.size());
    for (auto &config : configs)
    {
        TokenInstrument instrument;
        instrument.contract_id = std::move(config.contract.contractId);
        instrument.created_at = std::move(config.contract.createdAt.value_or(""));
        instrument.operator_party = std::move(config.payload.instr_operator);
        instrument.provider_party = std::move(config.payload.provider);
        instrument.registrar_party = std::move(config.payload.registrar);
        instrument.scheme = std::move(config.payload.defaultIdentifier.scheme);
        instrument.token_id = std::move(config.payload.defaultIdentifier.id);

        const std::string &registrar = instrument.registrar_party;
        const std::string &token_id = instrument.token_id;

        if (!instruments.contains(registrar))
        {
            auto token_standard = SslConnection(m_cfg.utility_base_url, m_cfg.request_timeout);
            token_standard / "token-standard/v0/registrars" / registrar / "registry/metadata/v1/instruments";

            try
            {
                update_instruments_map_for_registrar(registrar, instruments, token_standard, m_cfg.page_size);
            }
            catch (std::exception &e)
            {
                spdlog::error(e.what());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!instruments.contains(registrar) || !instruments[registrar].contains(token_id))
        {
            spdlog::warn(std::format("[instruments scan] can't find instrument with token_id {} and registrar {}",
                                     token_id, registrar));
            continue;
        }
        const Instrument &instr = instruments[registrar][token_id];
        instrument.name = instr.name;
        instrument.symbol = instr.symbol;
        instrument.total_supply = instr.totalSupply.value_or("0");
        instrument.decimals = instr.decimals;

        parsed_instruments.push_back(std::move(instrument));
    }

    db::token::initialize_schema(*m_db);

    m_db->execute([&](pqxx::connection &c) {
        pqxx::work txn(c);
        for (auto &instrument : parsed_instruments)
        {
            spdlog::info(std::format("[instruments scan] token {} with registrar {}", instrument.token_id,
                                     instrument.registrar_party));
            db::token::insert_new_token(txn, instrument);
        }
        txn.commit();
    });
}
} // namespace token_scan
