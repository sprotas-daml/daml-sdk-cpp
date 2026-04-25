module;

#include <string>
#include <vector>

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

module database.hold;

import database;
import utils;
import daml.type;
import daml.api;
import daml.crypto;

namespace db::hold
{
using daml::decimal::decimal;
using namespace daml::api;
using namespace daml::crypto;

std::string_view prefix()
{
    static const std::string prefix = []() {
        std::string p;
        try
        {
            p = get_env_var("DT_PREFIX_HOLD");
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
        db::initialize_update_table(txn);

        if (get_env_var("DO_RESET") == "true")
        {
            txn.exec(std::format(R"(
      DROP TABLE IF EXISTS {0}holdings;
      DROP TABLE IF EXISTS {0}balance;
    )",
                                 prefix()));
        }
        txn.exec(std::format(R"(
      CREATE TABLE IF NOT EXISTS {0}parties (
        id BIGSERIAL PRIMARY KEY,
        party TEXT NOT NULL UNIQUE,
        is_local BOOL NOT NULL
      )
    )",
                             prefix()));
        txn.exec(std::format(R"(
      CREATE TABLE IF NOT EXISTS {0}page_hash (
        id INT PRIMARY KEY DEFAULT 1,
        CONSTRAINT singleton_check CHECK (id = 1),

        last_page_hash TEXT NOT NULL,
        total_amount INT NOT NULL,
        updated_at TIMESTAMPTZ NOT NULL
      )
    )",
                             prefix()));

        txn.exec(std::format(R"(
      CREATE TABLE IF NOT EXISTS {0}holdings (
        id BIGSERIAL PRIMARY KEY,
        owner_party TEXT NOT NULL,
        token_id TEXT NOT NULL,
        amount NUMERIC(28,10) NOT NULL,
        is_locked BOOL NOT NULL DEFAULT FALSE,
        admin_party TEXT NOT NULL,
        contract_id TEXT NOT NULL UNIQUE,
        created_at TIMESTAMPTZ NOT NULL
        )
      )",
                             prefix()));

        txn.exec(std::format(R"(
      ALTER TABLE {0}holdings
        ALTER COLUMN amount TYPE NUMERIC(38,10)
    )",
                             prefix()));

        txn.exec(std::format(R"(
      ALTER TABLE {0}holdings SET (
          autovacuum_vacuum_scale_factor = 0.02,
          autovacuum_analyze_scale_factor = 0.01,
          autovacuum_vacuum_threshold = 1000
      )
    )",
                             prefix()));

        txn.exec(std::format(R"(
      CREATE TABLE IF NOT EXISTS {0}balance (
        owner_party TEXT NOT NULL,
        token_id TEXT NOT NULL,
        amount NUMERIC(28,10) NOT NULL DEFAULT 0,
        locked_amount NUMERIC(28,10) NOT NULL DEFAULT 0,
        admin_party TEXT NOT NULL,
        updated_at TIMESTAMPTZ NOT NULL,
        PRIMARY KEY (owner_party, admin_party, token_id)
      )
    )",
                             prefix()));
        txn.exec(std::format(R"(
      ALTER TABLE {0}balance
        ALTER COLUMN amount TYPE NUMERIC(38,10),
        ALTER COLUMN locked_amount TYPE NUMERIC(38,10)

    )",
                             prefix()));
        txn.commit();
    });
    spdlog::info("[local db] holding schema initialized");
}

bool party_is_local(pqxx::work &txn, std::string_view party)
{
    pqxx::result is_local = txn.exec(std::format(R"(
              SELECT is_local FROM {0}parties
              WHERE party = $1
            )",
                                                 prefix()),
                                     pqxx::params{
                                         party,
                                     });
    if (!is_local.empty())
        return is_local[0]["is_local"].as<bool>();

    nlohmann::json party_info;
    try
    {
        party_info = client::ledger_get("v2/parties/" + std::string{party}, token::get_token());
    }
    catch (const std::exception &e)
    {
        spdlog::debug(e.what());
        return false;
    }

    auto party_details = party_info.at("partyDetails");
    for (auto &party_info : party_details)
    {
        if (party_info.at("party").get<std::string>() == party)
        {
            bool is_local = party_info.at("isLocal").get<bool>();
            txn.exec(std::format(R"(
          INSERT INTO {0}parties (party, is_local)
          VALUES ($1, $2)
        )",
                                 prefix()),
                     pqxx::params{
                         party,
                         is_local,
                     });
            return is_local;
        }
    }
    return false;
}

void upsert_party(pqxx::work &txn, std::string_view party, bool is_local)
{
    txn.exec(std::format(R"(
      INSERT INTO {0}parties (party, is_local)
      VALUES ($1, $2)
      ON CONFLICT(party) DO NOTHING
      )",
                         prefix()),
             pqxx::params{
                 party,
                 is_local,
             });
}

void upsert_page_hash(pqxx::work &txn, const std::string &hash, node_int_t amount)
{
    if (hash.empty() || amount == node_int_t{})
        return;

    txn.exec(std::format(R"(
      INSERT INTO {0}page_hash (id, last_page_hash, total_amount, updated_at)
      VALUES (1, $1, $2, NOW())
      ON CONFLICT(id) DO UPDATE SET
        last_page_hash = EXCLUDED.last_page_hash,
        total_amount = {0}page_hash.total_amount + EXCLUDED.total_amount,
        updated_at = NOW()
    )",
                         prefix()),
             pqxx::params{hash, amount});
}

void insert_holding_and_increase_balance(pqxx::work &txn, const Holding &holding)
{
    if (!party_is_local(txn, holding.owner_party))
    {
        spdlog::debug(std::format("[db holding] Party {} isn't local", holding.owner_party));
        return;
    }
    txn.exec(std::format(R"(
          INSERT INTO {}holdings (owner_party, token_id, amount, admin_party, contract_id, created_at, is_locked)
          VALUES ($1, $2, $3, $4, $5, $6::timestamptz, $7)
          ON CONFLICT(contract_id) DO NOTHING
        )",
                         prefix()),
             pqxx::params{holding.owner_party, holding.token_id, holding.amount.to_string(), holding.admin_party,
                          holding.contract_id, holding.created_at, holding.is_locked});

    static const std::string amount_req = std::format(R"(
      SELECT amount
      FROM {}balance
      WHERE owner_party = $1 AND token_id = $2 AND admin_party = $3 FOR UPDATE
    )",
                                                      prefix());
    static const std::string locked_amount_req = std::format(R"(
      SELECT locked_amount
      FROM {}balance
      WHERE owner_party = $1 AND token_id = $2 AND admin_party = $3 FOR UPDATE
    )",
                                                             prefix());
    std::string_view req = holding.is_locked ? locked_amount_req : amount_req;
    pqxx::result current_bal = txn.exec(req, pqxx::params{
                                                 holding.owner_party,
                                                 holding.token_id,
                                                 holding.admin_party,
                                             });
    if (current_bal.empty())
    {
        static const std::string insert_amount_req = std::format(R"(
          INSERT INTO {}balance (owner_party, token_id, amount, admin_party, updated_at)
          VALUES ($1, $2, $3, $4, NOW())
        )",
                                                                 prefix());
        static const std::string insert_locked_amount_req = std::format(R"(
          INSERT INTO {}balance (owner_party, token_id, locked_amount, admin_party, updated_at)
          VALUES ($1, $2, $3, $4, NOW())
        )",
                                                                        prefix());
        std::string_view req = holding.is_locked ? insert_locked_amount_req : insert_amount_req;
        txn.exec(req, pqxx::params{
                          holding.owner_party,
                          holding.token_id,
                          holding.amount.to_string(),
                          holding.admin_party,
                      });
    }
    else
    {
        decimal current_balance = decimal(current_bal[0][0].as<std::string>());
        decimal new_balance = current_balance + holding.amount;

        static const std::string update_balance_req = std::format(R"(
        UPDATE {}balance
        SET
          amount = $1,
          updated_at = NOW()
        WHERE owner_party = $2 AND token_id = $3 AND admin_party = $4
      )",
                                                                  prefix());
        static const std::string update_locked_balance_req = std::format(R"(
        UPDATE {}balance
        SET
          locked_amount = $1,
          updated_at = NOW()
        WHERE owner_party = $2 AND token_id = $3 AND admin_party = $4
      )",
                                                                         prefix());
        std::string_view req = holding.is_locked ? update_locked_balance_req : update_balance_req;
        txn.exec(req, pqxx::params{
                          new_balance.to_string(),
                          holding.owner_party,
                          holding.token_id,
                          holding.admin_party,
                      });
    }
}
void delete_holding_and_decrease_balance(pqxx::work &txn, const std::string &removed_holding)
{
    pqxx::result del_res = txn.exec(std::format(R"(
              DELETE FROM {}holdings WHERE contract_id = $1
              RETURNING owner_party, token_id, admin_party, amount, is_locked
    )",
                                                prefix()),
                                    pqxx::params{removed_holding});
    if (del_res.empty())
    {
        spdlog::debug("[db holding] Holding not found or already deleted: " + removed_holding);
        return;
    }

    std::string owner_party = del_res[0]["owner_party"].as<std::string>();
    std::string token_id = del_res[0]["token_id"].as<std::string>();
    std::string admin_party = del_res[0]["admin_party"].as<std::string>();
    decimal amount_to_subtract = decimal(del_res[0]["amount"].as<std::string>());
    bool is_locked = del_res[0]["is_locked"].as<bool>();

    static const std::string amount_req = std::format(R"(
      SELECT amount
      FROM {}balance
      WHERE owner_party = $1 AND token_id = $2 AND admin_party = $3 FOR UPDATE
    )",
                                                      prefix());
    static const std::string locked_amount_req = std::format(R"(
      SELECT locked_amount
      FROM {}balance
      WHERE owner_party = $1 AND token_id = $2 AND admin_party = $3 FOR UPDATE
    )",
                                                             prefix());
    std::string_view bal_req = is_locked ? locked_amount_req : amount_req;
    pqxx::result current_bal = txn.exec(bal_req, pqxx::params{
                                                     owner_party,
                                                     token_id,
                                                     admin_party,
                                                 });
    if (current_bal.empty())
    {
        throw std::runtime_error("Critical error: Balance record missing for existing holding");
    }

    decimal current_balance = decimal(current_bal[0][0].as<std::string>());
    decimal new_balance = current_balance - amount_to_subtract;

    static const std::string update_balance_req = std::format(R"(
      UPDATE {}balance
      SET
        amount = $1,
        updated_at = NOW()
      WHERE owner_party = $2 AND token_id = $3 AND admin_party = $4
    )",
                                                              prefix());
    static const std::string update_locked_balance_req = std::format(R"(
      UPDATE {}balance
      SET
        locked_amount = $1,
        updated_at = NOW()
      WHERE owner_party = $2 AND token_id = $3 AND admin_party = $4
    )",
                                                                     prefix());
    std::string_view update_req = is_locked ? update_locked_balance_req : update_balance_req;

    txn.exec(update_req, pqxx::params{
                             new_balance.to_string(),
                             owner_party,
                             token_id,
                             admin_party,
                         });
}

void insert_contracts_impl(pqxx::work &txn, const std::vector<Update> &updates)
{
    for (const auto &u : updates)
    {
        db::insert_update_row(txn, u.update_row);

        for (const auto &h : u.created_holdings)
        {
            insert_holding_and_increase_balance(txn, h);
        }
        for (const auto &rh : u.removed_holding)
        {
            delete_holding_and_decrease_balance(txn, rh);
        }
    }
}

void initialize_parties(Connection &conn)
{
    using namespace daml::api;
    static constexpr node_int_t page_size = 2000;

    pqxx::result res = conn.execute([](pqxx::connection &c) {
        pqxx::work txn(c);

        const std::string select_query =
            std::format("SELECT last_page_hash, total_amount FROM {0}page_hash WHERE id = 1", prefix());
        return txn.exec(select_query);
    });

    std::string hash{};
    node_int_t total_amount{};
    if (!res.empty())
    {
        const auto row = res[0];
        hash = row["last_page_hash"].as<std::string>();
        total_amount = row["total_amount"].as<node_int_t>();
    }

    spdlog::info(std::format("[party] start initializing party list. Current state: {} party parsed", total_amount));

    bool is_first_fetch = true;

    while (is_first_fetch || !hash.empty())
    {
        is_first_fetch = false;
        nlohmann::json parties = request::get_parties(token::get_token(), hash, page_size);
        hash = parties.at("nextPageToken");
        std::vector<nlohmann::json> party_details = parties.at("partyDetails");

        node_int_t current_amount = party_details.size();

        conn.execute([&](pqxx::connection &c) {
            pqxx::work txn(c);
            for (const auto &party : party_details)
            {
                std::string_view party_name = party.at("party").get<std::string_view>();
                bool is_local = party.at("isLocal").get<bool>();

                spdlog::trace(std::format("[party] parsed party: {}, is_local: {}", party_name, is_local));

                upsert_party(txn, party_name, is_local);
            }

            if (!hash.empty())
            {
                upsert_page_hash(txn, hash, current_amount);
            }
            total_amount += current_amount;
            spdlog::info(std::format("[party] current state : {}", total_amount));
            txn.commit();
        });
    }

    spdlog::info(std::format("[party] initialize party list finished. Total: {}", total_amount));
}

} // namespace db::hold
