module;

#include <functional>
#include <memory>
#include <optional>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <tuple>

module db_writer;

import config;
import manager;
import database;
import converter;

import pipeline.domain_traits;
import pipeline.router;

namespace db_writer
{
DbWriter::DbWriter(std::shared_ptr<db::Connection> conn) : m_conn(std::move(conn))
{
}

void DbWriter::write_batch(manager::ParsedBatch &&batch)
{
    if (!m_conn)
    {
        spdlog::error("[local db_writer] no database connection");
        return;
    }

    if (batch.updates.empty())
        return;

    spdlog::debug("[local db_writer] Starting processing batch with {} updates", batch.updates.size());

    using namespace pipeline;

    auto make_batches = []<typename... Ts>(traits::TypeList<Ts...>) { return std::tuple<std::vector<Ts>...>{}; };
    auto db_batches = make_batches(traits::RegisteredDomains{});

    auto process_domain = [&db_batches]<typename T>(router::DomainRawUpdate<T> &wrapper) {
        if (router::has_changes(wrapper.data))
        {
            spdlog::trace("[local db_writer] Converting update for domain type: {}", traits::domain_name_v<T>);
            std::get<std::vector<T>>(db_batches).emplace_back(converter::to_db<T>(std::move(wrapper.data)));
        }
    };

    auto insert_domain = []<typename T>(pqxx::work &txn, const std::vector<T> &vec) {
        if (!vec.empty())
        {
            spdlog::debug("[local db_writer] Inserting {} aggregated updates for domain type: {}", vec.size(),
                          traits::domain_name_v<T>);
            db::insert_contracts<T>(txn, vec);
        }
    };

    spdlog::debug("[local db_writer] Splitting and converting raw updates...");
    for (auto &raw_update : batch.updates)
    {
        auto split_result = router::split_update(std::move(raw_update), traits::RegisteredDomains{});

        std::apply([&](auto &...wrappers) { (process_domain(wrappers), ...); }, split_result);
    }

    spdlog::debug("[local db_writer] Committing converted batches to DB...");

    m_conn->execute([&](pqxx::connection &c) {
        pqxx::work txn(c);
        std::apply([&](const auto &...vecs) { (insert_domain(txn, vecs), ...); }, db_batches);

        txn.commit();
    });

    spdlog::debug("[local db_writer] Batch transaction committed successfully");
}
} // namespace db_writer
