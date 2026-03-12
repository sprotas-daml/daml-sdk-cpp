module;

#include <atomic>
#include <chrono>
#include <optional>
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
    static void init(NodeConfig ledger, NodeConfig scan);
    
    [[nodiscard]] static const NodeConfig &get_ledger();
    [[nodiscard]] static const NodeConfig &get_scan();

  private:
    static void ensure_initialized();

    static NodeConfig m_ledger;
    static NodeConfig m_scan;
    static std::atomic<bool> m_initialized;
};
} // namespace daml::api::registry