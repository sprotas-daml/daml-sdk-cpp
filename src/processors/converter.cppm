export module converter;

import manager;
import database;

export namespace converter
{
template <typename T> T to_db(manager::ParsedUpdate &&src);

db::UpdateRow to_db(const manager::ParsedUpdateInfo &src);
db::UpdateRow to_db(manager::ParsedUpdateInfo &&src);

} // namespace converter