module;

#include <atomic>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>

export module daml.api:registry;

export namespace daml::api::registry
{
struct NodeConfig
{
    std::string url;
    std::optional<std::chrono::milliseconds> timeout;
};

class Registry
{
  public:
    static void init(NodeConfig ledger, NodeConfig scan)
    {
        if (m_initialized.load(std::memory_order_relaxed))
        {
            throw std::runtime_error("[registry]: already initialized!");
        }

        m_ledger = std::move(ledger);
        m_scan = std::move(scan);

        m_initialized.store(true, std::memory_order_release);
    }

    [[nodiscard]] static const NodeConfig &get_ledger()
    {
        ensure_initialized();
        return m_ledger;
    }

    [[nodiscard]] static const NodeConfig &get_scan()
    {
        ensure_initialized();
        return m_scan;
    }

  private:
    static void ensure_initialized()
    {
        if (!m_initialized.load(std::memory_order_acquire))
        {
            throw std::runtime_error("[registry]: access before initialization!");
        }
    }

    inline static NodeConfig m_ledger;
    inline static NodeConfig m_scan;
    inline static std::atomic<bool> m_initialized{false};
};
} // namespace daml::api::registry
