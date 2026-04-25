module;

#include <cstdint>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>
#include <pqxx/pqxx>

export module database;

import utils;

export namespace db
{
class Connection
{
  public:
    explicit Connection(std::string conn_str);

    template <typename Func> auto execute(Func &&db_work, int max_retries = 5)
    {
        int attempt = 0;

        while (true)
        {
            try
            {
                return db_work(*m_conn);
            }
            catch (const pqxx::broken_connection &e)
            {
                attempt++;
                spdlog::warn("[db] database unavailable, attempt to reconnect {}", attempt);
                if (attempt >= max_retries)
                {
                    spdlog::error("[db] database unavailable");
                    throw;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                try
                {
                    connect();
                }
                catch (const std::exception &conn_e)
                {
                    spdlog::error("[db] can't reconnect {}", conn_e.what());
                }
            }
        }
    }

  private:
    std::string m_conn_str;
    std::unique_ptr<pqxx::connection> m_conn;

    void connect();
};

std::string_view prefix()
{
    static const std::string prefix = []() {
        std::string p;
        try
        {
            p = get_env_var("DT_PREFIX");
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

struct UpdateRow
{
    std::int64_t offset{0};
    std::string update_id;
    std::string effective_at; // ISO 8601
};

inline void clear_update_table(Connection &conn)
{
    conn.execute([](pqxx::connection &c) {
        pqxx::work txn(c);

        txn.exec(fmt::format(R"(
          DO $$
          BEGIN
              TRUNCATE TABLE {0}updates RESTART IDENTITY CASCADE;
          EXCEPTION
              WHEN undefined_table THEN
                  NULL;
          END $$;
      )",
                             prefix()));
        txn.commit();
    });
}

inline void initialize_update_table(pqxx::work &txn)
{
    txn.exec(fmt::format(R"(
          CREATE TABLE IF NOT EXISTS {}updates (
              id BIGSERIAL PRIMARY KEY,
              update_id TEXT NOT NULL UNIQUE,
              ledger_offset BIGINT NOT NULL,
              effective_at TIMESTAMPTZ NOT NULL,
              created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
          )
  )",
                         prefix()));
}

inline void insert_update_row(pqxx::work &txn, const UpdateRow &update_row)
{
    txn.exec(fmt::format(
                 R"(
        INSERT INTO {}updates (update_id, ledger_offset, effective_at)
        VALUES ($1, $2, $3::timestamptz)
        ON CONFLICT (update_id) DO NOTHING
      )",
                 prefix()),
             pqxx::params{update_row.update_id, update_row.offset, update_row.effective_at});
}

template <typename T> void insert_contracts(pqxx::work &txn, const std::vector<T> &updates) {};

inline std::optional<std::int64_t> get_parsed_ledger_end(Connection &conn)
{
    return conn.execute([](pqxx::connection &c) -> std::optional<std::int64_t> {
        pqxx::read_transaction txn(c);
        auto res = txn.exec(fmt::format(R"(SELECT MAX(ledger_offset) FROM {}updates;)", prefix())).one_field();

        if (res.is_null())
        {
            return std::nullopt;
        }

        return res.as<std::uint64_t>();
    });
}

} // namespace db
