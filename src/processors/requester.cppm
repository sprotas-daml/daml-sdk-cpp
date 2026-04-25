module;

#include <chrono>
#include <memory>

export module requester;

import config;
import manager;

import daml.type;
import daml.model;

namespace requester
{
using namespace daml::model::response;

export class Requester
{
  public:
    enum class FetchStatus
    {
        Ok,
        TooLarge,
        Empty,
        Failed,
        ReachedEnd
    };

    struct RawBatch
    {
        FetchStatus status{FetchStatus::Failed};
        manager::PollWindow window{};
        std::vector<Update> payload;
    };

    Requester(std::shared_ptr<manager::Manager> manager, config::PollerConfig cfg);

    RawBatch fetch_window(manager::PollWindow window);
    std::optional<node_int_t> get_ledger_end();

  private:
    FetchStatus fetch_batch(node_int_t begin, node_int_t end, std::vector<Update> &out);

    std::shared_ptr<manager::Manager> m_manager;
    config::PollerConfig m_cfg;
};
} // namespace requester
