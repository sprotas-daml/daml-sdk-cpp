module;

#include <atomic>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

export module manager;

import config;
import utils.concurrent_queue;

import daml.model;
import daml.type;

namespace manager
{
using namespace daml::model;

export struct PollWindow
{
    std::uint64_t begin_exclusive{0};
    std::uint64_t end_inclusive{0};
};

export struct RawBatch
{
    std::uint64_t sequence_id;
    std::vector<daml::model::response::Update> payload;
    PollWindow window{};
};

export struct ParsedContract
{
    std::string template_id;
    std::string contract_id;
    std::string raw_blob; // createdEventBlob (base64)
    std::string created_at;
    nlohmann::json args; // JSON string
    std::vector<std::string> interface_ids;
    std::unordered_map<std::string, nlohmann::json> interface_views;
};

export struct ParsedChoice
{
    std::string template_id;
    std::string contract_id;
    std::string choice;
    bool consuming;
    nlohmann::json args;
    nlohmann::json result;
    std::vector<std::string> interface_ids;
};

export struct ParsedArchive
{
    std::string template_id;
    std::string contract_id;
    std::vector<std::string> interface_ids;
};

export struct ParsedUpdateInfo
{
    std::int64_t offset{0};
    std::string update_id;
    std::string effective_at; // ISO 8601
};

export struct ParsedUpdate
{
    ParsedUpdateInfo update_info;
    std::vector<ParsedContract> created_contracts;
    std::vector<ParsedChoice> choices;
    std::vector<ParsedArchive> archived_contracts;
};

export struct ParsedBatch
{
    std::uint64_t sequence_id;
    PollWindow window{};
    std::vector<ParsedUpdate> updates;
    std::int64_t last_offset{0};
};

export class Manager
{
  public:
    explicit Manager(const config::QueueConfig &cfg);

    void request_stop();

    [[nodiscard]] bool stop_requested() const;

    void close_queues();

    utils::ConcurrentQueue<RawBatch> m_raw_queue;
    utils::ConcurrentQueue<ParsedBatch> m_parsed_queue;

  private:
    std::atomic<bool> m_stop_requested{false};
};
} // namespace manager

export using manager::Manager;
