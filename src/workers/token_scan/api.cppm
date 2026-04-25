module;
#include <crow/app.h>
#include <crow/common.h>
#include <crow/http_request.h>
#include <crow/routing.h>
#include <memory>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

export module api:token_scan;

import database.token;
import :core;

export namespace api::token_scan
{

crow::json::wvalue response(const std::shared_ptr<db::Connection> &m_conn, int limit, int page)
{
    auto result = m_conn->execute([&](pqxx::connection &c) {
        pqxx::read_transaction txn(c);

        int offset = (page - 1) * limit;

        auto result = db::token::get_paginated_tokens(txn, limit, offset);
        txn.commit();
        return result;
    });

    crow::json::wvalue response;
    std::vector<crow::json::wvalue> response_items;
    for (const auto &item : result.items)
    {
        crow::json::wvalue json_item;
        json_item["contract_id"] = item.contract_id;
        json_item["token_id"] = item.token_id;
        json_item["token_name"] = item.name;
        json_item["token_symbol"] = item.symbol;
        json_item["decimals"] = item.decimals;
        json_item["registrar"] = item.registrar_party;
        response_items.push_back(std::move(json_item));
    }

    if (response_items.empty())
    {
        response["tokens"] = crow::json::wvalue::list();
    }
    else
    {
        response["tokens"] = std::move(response_items);
    }

    if (result.has_next)
    {
        response["next_page"] = page + 1;
    }
    else
    {
        response["next_page"] = nullptr;
    }

    return response;
}

api::Router init_route(const std::shared_ptr<db::Connection> &m_conn)
{
    auto router = std::make_shared<crow::Blueprint>("token_scan");

    CROW_BP_ROUTE((*router), "/all").methods(crow::HTTPMethod::GET)([m_conn](const crow::request &req) {
        int page = 1;
        int limit = 100;
        try
        {
            if (req.url_params.get("page") != nullptr)
            {
                page = std::max(1, std::stoi(req.url_params.get("page")));
            }
            if (req.url_params.get("page_size") != nullptr)
            {
                limit = std::max(1, std::stoi(req.url_params.get("page_size")));
            }
        }
        catch (const std::exception &e)
        {
            return crow::response(400, "Invalid pagination parameters");
        };

        crow::json::wvalue resp;
        try
        {
            resp = response(m_conn, limit, page);
        }
        catch (const std::exception &e)
        {
            spdlog::error(std::format("Database error: {}", e.what()));
            return crow::response(500, "Internal Database Error");
        }

        return crow::response(200, resp);
    });

    return router;
}

} // namespace api::token_scan
