module;

#include <memory>
#include <string>
#include <vector>

export module parser;

import config;
import manager;

import daml.model;

namespace parser
{
using namespace daml::model::response;

export class Parser final
{
  public:
    Parser() = delete;
    static manager::ParsedBatch parse(const manager::PollWindow &window, std::vector<Update> &&payload);

  private:
    static std::int64_t parse_updates(std::vector<Update> &&payload,
                                      std::vector<manager::ParsedUpdate> &parsed_updates);
};
} // namespace parser
