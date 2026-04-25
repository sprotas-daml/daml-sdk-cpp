module;

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

module converter.hold;

import converter;
import manager;
import database;
import database.hold;

import daml.model;
import daml.type;

namespace converter::hold
{
using namespace daml::model::datatype;
using daml::decimal::decimal;

db::hold::Holding to_db(const manager::ParsedContract &contract)
{
    db::hold::Holding holding;
    holding.contract_id = contract.contract_id;
    holding.created_at = contract.created_at;

    for (auto &[key, value] : contract.interface_views)
    {
        if (!key.ends_with("Holding"))
            continue;
        HoldingView holding_view = std::move(value);
        holding.admin_party = std::move(holding_view.instrumentId.admin);
        holding.token_id = std::move(holding_view.instrumentId.id);
        holding.owner_party = std::move(holding_view.owner);
        holding.amount = decimal(holding_view.amount);
        holding.is_locked = holding_view.lock.has_value();
        return holding;
    }

    throw std::runtime_error("[holdings] error in created holding parsing");
}

db::hold::Holding to_db(manager::ParsedContract &&contract)
{
    db::hold::Holding holding;
    holding.contract_id = std::move(contract.contract_id);
    holding.created_at = std::move(contract.created_at);

    for (auto &[key, value] : contract.interface_views)
    {
        if (!key.ends_with("Holding"))
            continue;
        HoldingView holding_view = std::move(value);
        holding.admin_party = std::move(holding_view.instrumentId.admin);
        holding.token_id = std::move(holding_view.instrumentId.id);
        holding.owner_party = std::move(holding_view.owner);
        holding.amount = decimal(holding_view.amount);
        holding.is_locked = holding_view.lock.has_value();
        return holding;
    }

    throw std::runtime_error("[holdings] error in created holding parsing");
}

std::string to_db(const manager::ParsedChoice &choice)
{
    if (!choice.consuming)
        throw;
    return choice.contract_id;
}

std::string to_db(manager::ParsedChoice &&choice)
{
    if (!choice.consuming)
        throw;
    return std::move(choice.contract_id);
}

std::string to_db(const manager::ParsedArchive &choice)
{
    return choice.contract_id;
}

std::string to_db(manager::ParsedArchive &&choice)
{
    return std::move(choice.contract_id);
}

db::hold::Update to_db_impl(manager::ParsedUpdate &&src)
{
    db::hold::Update result;
    result.update_row = converter::to_db(std::move(src.update_info));

    for (auto &created : src.created_contracts)
    {
        result.created_holdings.emplace_back(to_db(std::move(created)));
    }
    for (auto &choice : src.choices)
    {
        try
        {
            result.removed_holding.emplace_back(to_db(std::move(choice)));
        }
        catch (...)
        {
            continue;
        }
    }
    return result;
}
} // namespace converter::hold

namespace converter
{
template <> db::hold::Update to_db<db::hold::Update>(manager::ParsedUpdate &&src)
{
    return hold::to_db_impl(std::move(src));
}
} // namespace converter
