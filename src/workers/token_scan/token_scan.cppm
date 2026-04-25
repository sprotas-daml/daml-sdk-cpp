module;
#include <memory>
#include <unordered_map>

export module token_scan;

import config;
import database;

export namespace token_scan
{

class TokenScanner
{
  public:
    TokenScanner(config::ScanConfig cfg, std::shared_ptr<db::Connection> m_db);
    void run_once();

  private:
    config::ScanConfig m_cfg;

    std::shared_ptr<db::Connection> m_db;
};
} // namespace token_scan
