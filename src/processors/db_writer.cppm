module;

#include <memory>

export module db_writer;

import config;
import manager;
import database;

namespace db_writer
{
export class DbWriter
{
  public:
    DbWriter(std::shared_ptr<db::Connection> conn);

    void write_batch(manager::ParsedBatch &&batch);

  private:
    std::shared_ptr<db::Connection> m_conn;
};
} // namespace db_writer
