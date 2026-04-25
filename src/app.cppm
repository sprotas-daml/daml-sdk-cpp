module;

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

export module app;

import config;
import db_writer;
import database;
import manager;
import requester;
import token_scan;
import api;

import daml.type;

namespace app
{
using namespace daml;

export class Runtime
{
  public:
    Runtime(std::string conn_str, config::AppConfig cfg);
    ~Runtime();

    void start();
    void stop();

  private:
    void requester_worker();
    void parser_worker();
    void writer_worker();
    void token_scanner_worker();

    config::AppConfig m_cfg;
    std::shared_ptr<manager::Manager> m_manager;
    std::shared_ptr<db::Connection> m_db;
    std::shared_ptr<requester::Requester> m_requester;
    std::shared_ptr<db_writer::DbWriter> m_writer;
    std::shared_ptr<token_scan::TokenScanner> m_token_scanner;
    std::unique_ptr<api::Server> m_api;
    std::vector<std::thread> m_requester_threads;
    std::vector<std::thread> m_parser_threads;
    std::thread m_writer_thread;
    std::thread m_token_scanner_thread;

    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_started{false};

    // State for window management, moved from Requester
    std::mutex m_work_mutex;
    node_int_t m_cursor_begin{0};
    node_int_t m_current_distance{0};
    node_int_t m_known_ledger_end{0};
    std::chrono::steady_clock::time_point m_last_ledger_end_fetch{};
    int m_successful_requests_count{0};
    std::vector<manager::PollWindow> m_gaps;

    std::mutex m_stop_mutex;
    std::condition_variable m_stop_cv;
};
} // namespace app
