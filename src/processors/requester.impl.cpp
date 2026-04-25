module;

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <cpr/cpr.h>
#include <cpr/response.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

module requester;

import config;
import manager;
import utils;

import daml;

namespace requester
{
using namespace std::chrono_literals;
using namespace daml::api;
using namespace daml::crypto;
using namespace daml::model::response;

Requester::Requester(std::shared_ptr<manager::Manager> manager, config::PollerConfig cfg)
    : m_manager(std::move(manager)), m_cfg(std::move(cfg))
{
    if (m_cfg.template_ids.empty())
    {
        spdlog::warn("[local requester] template_ids not configured");
    }
    if (m_cfg.interface_ids.empty())
    {
        spdlog::warn("[local requester] interface_ids not configured");
    }
}

std::optional<node_int_t> Requester::get_ledger_end()
{
    try
    {
        return request::get_ledger_end(token::get_token());
    }
    catch (const std::exception &e)
    {
        spdlog::error("[requester] get_ledger_end exception: {}", e.what());
        return std::nullopt;
    }
}

Requester::RawBatch Requester::fetch_window(manager::PollWindow window)
{
    RawBatch out;
    out.window = window;

    if (m_manager && m_manager->stop_requested())
    {
        out.status = FetchStatus::Failed;
        return out;
    }

    if (out.window.end_inclusive <= out.window.begin_exclusive)
    {
        spdlog::warn("[local requester] window collapsed, begin={}, end={}", out.window.begin_exclusive,
                     out.window.end_inclusive);
        out.status = FetchStatus::Failed; // Or some other appropriate status
        return out;
    }

    out.status = fetch_batch(out.window.begin_exclusive, out.window.end_inclusive, out.payload);

    while (out.status == FetchStatus::TooLarge)
    {
        // Reduce window and retry
        auto new_end = out.window.begin_exclusive + (out.window.end_inclusive - out.window.begin_exclusive) / 2;
        if (new_end <= out.window.begin_exclusive)
        {
            spdlog::error("[local requester] window cannot be reduced further but is still too large");
            out.status = FetchStatus::Failed;
            break;
        }
        out.window.end_inclusive = new_end;
        spdlog::warn(std::format("[local requester] retry request with smaller window size [{}, {}]",
                                 out.window.begin_exclusive, out.window.end_inclusive));

        out.status = fetch_batch(out.window.begin_exclusive, out.window.end_inclusive, out.payload);
    }

    return out;
}

Requester::FetchStatus Requester::fetch_batch(node_int_t begin, node_int_t end, std::vector<Update> &out)
{
    if (m_cfg.node_base_url.empty())
    {
        spdlog::error("[local requester] base_url not configured");
        return FetchStatus::Failed;
    }

    try
    {
        out = request::get_updates_flats_by_template_and_interface(token::get_token(), begin, end, m_cfg.template_ids,
                                                                   m_cfg.interface_ids);
    }
    catch (std::exception &e)
    {
        std::string message = e.what();

        if (message.find("JSON_API_MAXIMUM_LIST_ELEMENTS_NUMBER_REACHED") != std::string::npos)
        {
            spdlog::warn("[local requester] server reported maximum list elements reached");
            return FetchStatus::TooLarge;
        }

        const auto preview = message.substr(0, 1024);

        if (message.find("OFFSET_AFTER_LEDGER_END") != std::string::npos)
        {
            spdlog::warn("[local requester] request window was beyond ledger end");
            return FetchStatus::ReachedEnd;
        }

        return FetchStatus::Failed;
    }

    return out.empty() ? FetchStatus::Empty : FetchStatus::Ok;
}
} // namespace requester
