module;

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

export module database.token;

import database;

export namespace db::token
{
struct TokenInstrument
{
    std::string contract_id;
    std::string token_id;
    std::string name;
    std::string symbol;
    std::string total_supply;
    std::int32_t decimals;
    std::string operator_party;
    std::string provider_party;
    std::string registrar_party;
    std::string scheme;
    std::string created_at;
};

std::string_view prefix();

void initialize_schema(Connection &conn);
void insert_new_token(pqxx::work &txn, const TokenInstrument &token);

struct PaginatedTokens {
  std::vector<TokenInstrument> items;
  bool has_next;
};

PaginatedTokens get_paginated_tokens(pqxx::read_transaction &txn, int limit, int offset);

}; // namespace db::token
