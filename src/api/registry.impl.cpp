module;

#include <atomic>
#include <stdexcept>
#include <utility>

module daml.api;

namespace daml::api::registry
{
NodeConfig Registry::m_ledger;
NodeConfig Registry::m_scan;
std::atomic<bool> Registry::m_initialized{false};

void Registry::init(NodeConfig ledger, NodeConfig scan)
{
    if (m_initialized.load(std::memory_order_relaxed))
    {
        throw std::runtime_error("[registry]: already initialized!");
    }

    m_ledger = std::move(ledger);
    m_scan = std::move(scan);

    m_initialized.store(true, std::memory_order_release);
}

const NodeConfig &Registry::get_ledger()
{
    ensure_initialized();
    return m_ledger;
}

const NodeConfig &Registry::get_scan()
{
    ensure_initialized();
    return m_scan;
}

void Registry::ensure_initialized()
{
    if (!m_initialized.load(std::memory_order_acquire))
    {
        throw std::runtime_error("[registry]: access before initialization!");
    }
}
} // namespace daml::api::registry