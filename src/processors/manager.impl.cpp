module;

#include <atomic>

module manager;

import config;

namespace manager
{

Manager::Manager(const config::QueueConfig &cfg)
    : m_raw_queue(cfg.raw_queue_capacity), m_parsed_queue(cfg.parsed_queue_capacity)
{
}

void Manager::request_stop()
{
    m_stop_requested.store(true, std::memory_order_relaxed);
}

bool Manager::stop_requested() const
{
    return m_stop_requested.load(std::memory_order_relaxed);
}

void Manager::close_queues()
{
    m_raw_queue.close();
    m_parsed_queue.close();
}

} // namespace manager
