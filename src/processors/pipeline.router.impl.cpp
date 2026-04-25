module;

#include <vector>

module pipeline.router;

import manager;

namespace pipeline::router
{
bool has_changes(const manager::ParsedUpdate &u)
{
    return !u.created_contracts.empty() || !u.choices.empty();
}
} // namespace pipeline::router
