module;

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

module app;

import config;
import db_writer;
import database;
import database.hold;
import database.token;
import manager;
import parser;
import requester;
import utils;

namespace app
{
using namespace std::chrono_literals;

Runtime::Runtime(std::string conn_str, config::AppConfig cfg)
    : m_cfg(std::move(cfg)), m_manager(std::make_shared<manager::Manager>(m_cfg.queues)),
      m_db(std::make_shared<db::Connection>(std::move(conn_str))),
      m_requester(std::make_shared<requester::Requester>(m_manager, m_cfg.poller)),
      m_writer(std::make_shared<db_writer::DbWriter>(m_db)),
      m_token_scanner(std::make_shared<token_scan::TokenScanner>(m_cfg.token_scan, m_db)),
      m_current_distance(m_cfg.poller.default_poll_offset_distance)
{
    if (get_env_var("DO_RESET") == "true")
    {
        spdlog::debug("[local app] reset update counter");
        db::clear_update_table(*m_db);
    }
    db::hold::initialize_schema(*m_db);
    db::hold::initialize_parties(*m_db);

    auto last_ledger_end = db::get_parsed_ledger_end(*m_db);
    if (last_ledger_end.has_value())
    {
        m_cursor_begin = last_ledger_end.value();
        spdlog::info("[local app] starting from offset {}", m_cursor_begin);
    }
    m_api = std::make_unique<api::Server>(m_db);
    m_api->setup_api_routes({api::token_scan::init_route});
}

Runtime::~Runtime()
{
    stop();
}

void Runtime::start()
{
    if (m_started.exchange(true))
        return;

    m_stop.store(false);

    m_writer_thread = std::thread([this]() { writer_worker(); });

    m_parser_threads.reserve(m_cfg.parser.max_threads);
    for (size_t i = 0; i < m_cfg.parser.max_threads; ++i)
    {
        m_parser_threads.emplace_back([this]() { parser_worker(); });
    }

    m_requester_threads.reserve(m_cfg.poller.max_threads);
    for (size_t i = 0; i < m_cfg.poller.max_threads; ++i)
    {
        m_requester_threads.emplace_back([this]() { requester_worker(); });
    }

    m_token_scanner_thread = std::thread([this]() { token_scanner_worker(); });
    
    m_api->start_async(m_cfg.service.host, m_cfg.service.port);
}

void Runtime::stop()
{
    if (!m_started.exchange(false))
        return;

    spdlog::info("Stop requested, shutting down pipeline...");

    m_manager->request_stop();
    m_manager->close_queues();

    m_stop_cv.notify_all();

    for (auto &t : m_requester_threads)
    {
        if (t.joinable())
            t.join();
    }
    for (auto &t : m_parser_threads)
    {
        if (t.joinable())
            t.join();
    }
    if (m_writer_thread.joinable())
        m_writer_thread.join();

    if (m_token_scanner_thread.joinable())
        m_token_scanner_thread.join();
    
    m_api->stop();

    spdlog::info("Pipeline shut down.");
}

void Runtime::requester_worker()
{
    spdlog::info("[requester] starting worker");

    while (!m_manager->stop_requested())
    {
        manager::PollWindow window_to_fetch;
        bool have_work = false;

        // 1. Get work (atomically)
        {
            std::unique_lock<std::mutex> lock(m_work_mutex);

            if (!m_gaps.empty())
            {
                window_to_fetch = m_gaps.back();
                m_gaps.pop_back();
                have_work = true;
                spdlog::debug("[requester] fetching from gap [{}, {}]", window_to_fetch.begin_exclusive,
                              window_to_fetch.end_inclusive);
            }
            else
            {
                const auto now = std::chrono::steady_clock::now();
                if (m_known_ledger_end == 0 && (now - m_last_ledger_end_fetch >= m_cfg.poller.relax_delay))
                {
                    lock.unlock(); // Unlock while fetching ledger end
                    if (auto ledger_end = m_requester->get_ledger_end())
                    {
                        lock.lock();
                        m_known_ledger_end = *ledger_end;
                        m_last_ledger_end_fetch = now;
                        spdlog::info("[requester] fetched ledger end: {}", m_known_ledger_end);
                    }
                    else
                    {
                        lock.lock(); // Re-lock before continuing
                    }
                }

                if (m_known_ledger_end == 0)
                {
                    have_work = false;
                }
                else if (m_cursor_begin >= m_known_ledger_end)
                {
                    m_known_ledger_end = 0; // Force refetch on next cycle
                    have_work = false;
                }
                else
                {
                    window_to_fetch.begin_exclusive = m_cursor_begin;
                    window_to_fetch.end_inclusive = m_cursor_begin + m_current_distance;

                    if (window_to_fetch.end_inclusive > m_known_ledger_end)
                    {
                        window_to_fetch.end_inclusive = m_known_ledger_end;
                    }

                    if (window_to_fetch.end_inclusive <= window_to_fetch.begin_exclusive)
                    {
                        m_known_ledger_end = 0; // Force refetch
                        have_work = false;
                    }
                    else
                    {
                        m_cursor_begin = window_to_fetch.end_inclusive; // Optimistic advance
                        have_work = true;
                    }
                }
            }
        } // Mutex unlocked

        if (!have_work)
        {
            std::this_thread::sleep_for(m_cfg.poller.relax_delay);
            continue;
        }

        // 2. Do the fetch (in parallel)
        auto result = m_requester->fetch_window(window_to_fetch);

        // 3. Process result and update shared state
        if (result.window.end_inclusive < window_to_fetch.end_inclusive)
        {
            manager::PollWindow new_gap;
            new_gap.begin_exclusive = result.window.end_inclusive;
            new_gap.end_inclusive = window_to_fetch.end_inclusive;

            std::lock_guard<std::mutex> lock(m_work_mutex);
            m_gaps.push_back(new_gap);
            spdlog::warn("[requester] partial fetch success, created gap [{}, {}]", new_gap.begin_exclusive,
                         new_gap.end_inclusive);
            m_current_distance = result.window.end_inclusive - result.window.begin_exclusive;
            m_successful_requests_count = 0;
            spdlog::warn("[requester] window size reduced to {} due to TooLarge response", m_current_distance);
        }

        if (result.status == requester::Requester::FetchStatus::Ok ||
            result.status == requester::Requester::FetchStatus::Empty)
        {
            std::lock_guard<std::mutex> lock(m_work_mutex);

            if (result.status == requester::Requester::FetchStatus::Empty)
            {
                spdlog::info("[requester] empty window [{}, {}]", result.window.begin_exclusive,
                             result.window.end_inclusive);
            }
            if (result.status == requester::Requester::FetchStatus::Ok)
            {
                spdlog::info("[requester] fetched window [{}, {}]", result.window.begin_exclusive,
                             result.window.end_inclusive);
            }
            m_successful_requests_count++;
            if (m_successful_requests_count >= m_cfg.poller.succesfull_attempts_before_double_distance)
            {
                m_successful_requests_count = 0;
                m_current_distance = std::min<std::uint64_t>(m_cfg.poller.max_offset_distance, m_current_distance * 2);
                spdlog::info("[requester] increasing window size to {}", m_current_distance);
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(m_work_mutex);
            m_gaps.push_back(window_to_fetch);
            spdlog::warn("[requester] fetch failed for [{}, {}], returned to gaps", window_to_fetch.begin_exclusive,
                         window_to_fetch.end_inclusive);

            m_successful_requests_count = 0;

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        manager::RawBatch batch_to_queue;
        batch_to_queue.sequence_id = result.window.begin_exclusive;
        batch_to_queue.payload = std::move(result.payload);
        batch_to_queue.window = result.window;

        m_manager->m_raw_queue.push(std::move(batch_to_queue));
    }
    spdlog::info("[requester] worker finished");
}

void Runtime::parser_worker()
{
    spdlog::info("[parser] starting worker");
    while (true)
    {
        auto raw_batch_opt = m_manager->m_raw_queue.pop();
        if (!raw_batch_opt)
        {
            break; // Queue closed
        }
        auto raw_batch = std::move(*raw_batch_opt);

        if (raw_batch.payload.empty())
        {
            manager::ParsedBatch parsed_batch;
            parsed_batch.sequence_id = raw_batch.sequence_id;
            parsed_batch.window = raw_batch.window;
            m_manager->m_parsed_queue.push(std::move(parsed_batch));
            continue;
        }

        try
        {
            auto parsed_data = parser::Parser::parse(raw_batch.window, std::move(raw_batch.payload));
            parsed_data.sequence_id = raw_batch.sequence_id;
            m_manager->m_parsed_queue.push(std::move(parsed_data));
        }
        catch (const std::exception &e)
        {
            spdlog::error("[parser] parsing exception for window [{},{}]: {}", raw_batch.window.begin_exclusive,
                          raw_batch.window.end_inclusive, e.what());
        }
    }
    spdlog::info("[parser] worker finished");
}

void Runtime::writer_worker()
{
    spdlog::info("[writer] starting worker");
    std::map<std::uint64_t, manager::ParsedBatch> reorder_buffer;
    std::uint64_t next_offset_to_write = m_cursor_begin;

    while (true)
    {
        auto parsed_batch_opt = m_manager->m_parsed_queue.pop();
        if (!parsed_batch_opt)
        {
            if (!reorder_buffer.empty())
            {
                spdlog::warn(
                    "[writer] queue closed, but {} items remain in reorder buffer. Final write may be incomplete.",
                    reorder_buffer.size());
            }
            break; // Queue closed
        }
        auto parsed_batch = std::move(*parsed_batch_opt);

        reorder_buffer.emplace(parsed_batch.sequence_id, std::move(parsed_batch));

        while (reorder_buffer.contains(next_offset_to_write))
        {
            auto batch_to_write = std::move(reorder_buffer.at(next_offset_to_write));
            reorder_buffer.erase(next_offset_to_write);

            next_offset_to_write = batch_to_write.window.end_inclusive;

            if (batch_to_write.updates.empty())
            {
                spdlog::debug("[writer] skipping empty batch for window [{},{}]", batch_to_write.window.begin_exclusive,
                              batch_to_write.window.end_inclusive);
                continue;
            }

            try
            {
                spdlog::info(std::format("[writer] write batch [{}, {}]", batch_to_write.window.begin_exclusive,
                                         batch_to_write.window.end_inclusive));
                m_writer->write_batch(std::move(batch_to_write));
            }
            catch (const std::exception &e)
            {
                spdlog::error("[writer] DB write exception: {}", e.what());
            }
        }
    }
    spdlog::info("[writer] worker finished");
}

void Runtime::token_scanner_worker()
{
    spdlog::info("[instruments scan] starting worker");

    while (!m_manager->stop_requested())
    {
        try
        {
            m_token_scanner->run_once();
        }
        catch (std::exception &e)
        {
            spdlog::error(e.what());
        }
        std::unique_lock<std::mutex> lock(m_stop_mutex);
        m_stop_cv.wait_for(lock, m_cfg.token_scan.rescan_timeout, [this]() { return m_manager->stop_requested(); });
    }
}

} // namespace app
