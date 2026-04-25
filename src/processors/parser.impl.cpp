module;

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

module parser;

import config;
import manager;

import daml.model;

namespace parser
{
using namespace daml::model::response;

manager::ParsedBatch Parser::parse(const manager::PollWindow &window, std::vector<Update> &&payload)
{
    manager::ParsedBatch parsed{};
    parsed.window = window;
    parsed.updates.clear();

    if (payload.empty())
    {
        spdlog::warn("[local parser] empty payload for window [{}, {}]", window.begin_exclusive, window.end_inclusive);
        return parsed;
    }

    try
    {
        parsed.last_offset = parse_updates(std::move(payload), parsed.updates);
    }
    catch (const std::exception &e)
    {
        spdlog::error("[local parser] parse error: {}", e.what());
    }

    return parsed;
}
std::int64_t Parser::parse_updates(std::vector<Update> &&payload, std::vector<manager::ParsedUpdate> &parsed_updates)
{
    std::int64_t max_offset = 0;
    parsed_updates.clear();

    for (auto &txnValue : payload)
    {
        manager::ParsedUpdate update;
        update.update_info.update_id = std::move(txnValue.updateId);
        update.update_info.effective_at = std::move(txnValue.effectiveAt);
        update.update_info.offset = txnValue.offset;

        if (update.update_info.offset > max_offset)
            max_offset = update.update_info.offset;

        for (auto &evWrapper : txnValue.events)
        {
            if (evWrapper.CreatedEvent.has_value())
            {
                auto dto = std::move(evWrapper.CreatedEvent).value();

                manager::ParsedContract c;

                c.contract_id = std::move(dto.contractId);
                c.template_id = std::move(dto.templateId);
                c.args = std::move(dto.createArgument);
                c.raw_blob = std::move(dto.createdEventBlob);
                c.created_at = std::move(dto.createdAt);

                c.interface_ids.reserve(dto.interfaceViews.size());
                for (auto &iv : dto.interfaceViews)
                {
                    c.interface_ids.push_back(iv.interfaceId);
                    c.interface_views.emplace(std::move(iv.interfaceId), std::move(iv.viewValue));
                }

                update.created_contracts.emplace_back(std::move(c));
            }

            if (evWrapper.ExercisedEvent.has_value())
            {
                auto dto = std::move(evWrapper.ExercisedEvent).value();
                update.choices.emplace_back(manager::ParsedChoice{
                    .template_id = std::move(dto.templateId),
                    .contract_id = std::move(dto.contractId),
                    .choice = std::move(dto.choice),
                    .consuming = dto.consuming,
                    .args = std::move(dto.choiceArgument),
                    .result = std::move(dto.exerciseResult),
                    .interface_ids = std::move(dto.implementedInterfaces),
                });
            }

            if (evWrapper.ArchivedEvent.has_value())
            {
                auto dto = std::move(evWrapper.ArchivedEvent).value();
                update.archived_contracts.emplace_back(manager::ParsedArchive{
                    .template_id = std::move(dto.templateId),
                    .contract_id = std::move(dto.contractId),
                    .interface_ids = std::move(dto.implementedInterfaces),
                });
            }
        }

        parsed_updates.emplace_back(std::move(update));
    }

    return max_offset;
}
} // namespace parser
