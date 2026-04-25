module;

#include <utility>

module converter;

import manager;
import database;

namespace converter
{
db::UpdateRow to_db(const manager::ParsedUpdateInfo &src)
{
    return {.offset = src.offset, .update_id = src.update_id, .effective_at = src.effective_at};
}

db::UpdateRow to_db(manager::ParsedUpdateInfo &&src)
{
    return {.offset = src.offset, .update_id = std::move(src.update_id), .effective_at = std::move(src.effective_at)};
}

} // namespace converter
