module;

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

export module database.hold;

import database;
import daml.type;

export namespace db::hold
{
using namespace daml;

std::string_view prefix();

struct Holding
{
    std::string owner_party;
    std::string token_id;
    decimal::decimal amount;
    bool is_locked;
    std::string admin_party;
    std::string contract_id;
    std::string created_at;
};

struct Update
{
    UpdateRow update_row;
    std::vector<Holding> created_holdings;
    std::vector<std::string> removed_holding;
};

void initialize_schema(Connection &conn);
void initialize_parties(Connection &conn);
void insert_contracts_impl(pqxx::work &txn, const std::vector<Update> &updates);

}; // namespace db::hold

export namespace db
{
template <> inline void insert_contracts<hold::Update>(pqxx::work &txn, const std::vector<hold::Update> &updates)
{
    spdlog::debug("[db hold]: write to db");
    db::hold::insert_contracts_impl(txn, updates);
}
} // namespace db
