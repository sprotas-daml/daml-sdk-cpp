module;

#include <string>

export module converter.hold;

import manager;
import database.hold;

export namespace converter::hold
{
db::hold::Holding to_db(const manager::ParsedContract &contract);
db::hold::Holding to_db(manager::ParsedContract &&contract);
std::string to_db(const manager::ParsedChoice &choice);
std::string to_db(manager::ParsedChoice &&choice);
db::hold::Update to_db_impl(manager::ParsedUpdate &&src);
} // namespace converter::hold
