module;

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

module database;

namespace db
{
Connection::Connection(std::string conn_str) : m_conn_str(std::move(conn_str))
{
    connect();
    if (!m_conn || !m_conn->is_open())
    {
        throw std::runtime_error("Failed to open database connection");
    }
    spdlog::info("[local db] connected to {}", m_conn->dbname());
}

void Connection::connect()
{
    if (m_conn && m_conn->is_open())
    {
        m_conn->close();
    }
    m_conn = std::make_unique<pqxx::connection>(m_conn_str);
}
} // namespace db
