module;

#include <cstdint>
#include <string>

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

module database.token;

import database;
import utils;

namespace db::token
{

std::string_view prefix()
{
    static const std::string prefix = []() {
        std::string p;
        try
        {
            p = get_env_var("DT_PREFIX_TOKEN");
        }
        catch (...)
        {
        }
        if (!p.empty() && p.back() != '_')
            p.push_back('_');
        return p;
    }();

    return prefix;
}

void initialize_schema(Connection &conn)
{
    conn.execute([](pqxx::connection &c) {
        pqxx::work txn(c);

        txn.exec(fmt::format(R"(
          DO $$
          BEGIN
              TRUNCATE TABLE {0}tokens RESTART IDENTITY CASCADE;
          EXCEPTION
              WHEN undefined_table THEN
                  NULL;
          END $$;
      )",
                             prefix()));
        txn.exec(std::format(R"(
      CREATE TABLE IF NOT EXISTS {0}tokens (
        contract_id TEXT NOT NULL PRIMARY KEY,
        created_at TIMESTAMPTZ NOT NULL,
        token_id TEXT NOT NULL,
        name TEXT NOT NULL,
        symbol TEXT NOT NULL,
        total_supply NUMERIC DEFAULT NULL,
        decimals INT NOT NULL,
        operator TEXT NOT NULL,
        provider TEXT NOT NULL,
        registrar TEXT NOT NULL,
        scheme TEXT NOT NULL
      )
    )",
                             prefix()));
        txn.commit();
    });
    spdlog::info("[local token] token schema initialized");
}

void insert_new_token(pqxx::work &txn, const TokenInstrument &token)
{
    if (token.created_at.empty())
    {
        txn.exec(std::format(R"(
      INSERT INTO {0}tokens
        (contract_id, created_at, token_id, name, symbol, total_supply, decimals, operator, provider, registrar, scheme)
      VALUES
        ($1, NOW(), $2, $3, $4, $5::numeric, $6, $7, $8, $9, $10)
    )",
                             prefix()),
                 pqxx::params{
                     token.contract_id,
                     token.token_id,
                     token.name,
                     token.symbol,
                     token.total_supply,
                     token.decimals,
                     token.operator_party,
                     token.provider_party,
                     token.registrar_party,
                     token.scheme,
                 });
        return;
    }
    txn.exec(std::format(R"(
    INSERT INTO {0}tokens
      (contract_id, created_at, token_id, name, symbol, total_supply, decimals, operator, provider, registrar, scheme)
    VALUES
      ($1, $11::timestamptz, $2, $3, $4, $5::numeric, $6, $7, $8, $9, $10)
  )",
                         prefix()),
             pqxx::params{
                 token.contract_id,
                 token.token_id,
                 token.name,
                 token.symbol,
                 token.total_supply,
                 token.decimals,
                 token.operator_party,
                 token.provider_party,
                 token.registrar_party,
                 token.scheme,
                 token.created_at,
             });
}

PaginatedTokens get_paginated_tokens(pqxx::read_transaction &txn, int limit, int offset)
{
    pqxx::result txn_result = txn.exec(std::format(R"(
    SELECT contract_id, created_at, token_id, name, symbol, total_supply, decimals, operator, provider, registrar, scheme
    FROM {0}tokens ORDER BY contract_id
    LIMIT $1 OFFSET $2
  )",
                                                   prefix()),
                                       pqxx::params{limit + 1, offset});

    PaginatedTokens result;
    for (auto row : txn_result)
    {
        result.items.push_back({
            .contract_id = row["contract_id"].as<std::string>(),
            .token_id = row["token_id"].as<std::string>(),
            .name = row["name"].as<std::string>(),
            .symbol = row["symbol"].as<std::string>(),
            .total_supply = row["total_supply"].as<std::string>(),
            .decimals = row["decimals"].as<std::int32_t>(),
            .operator_party = row["operator"].as<std::string>(),
            .provider_party = row["provider"].as<std::string>(),
            .registrar_party = row["registrar"].as<std::string>(),
            .scheme = row["scheme"].as<std::string>(),
            .created_at = row["created_at"].as<std::string>(),
        });
    }

    if (result.items.size() > static_cast<size_t>(limit))
    {
        result.has_next = true;
        result.items.pop_back();
    }
    else
    {
        result.has_next = false;
    }
    return result;
}
} // namespace db::token
