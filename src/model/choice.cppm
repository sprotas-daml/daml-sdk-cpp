module;

#include <cstdint>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

export module daml.model:choice;

import daml.type;

import :datatype;

export namespace daml::model::choice
{
using namespace daml::model::datatype;

struct AmuletRules_CreateTransferPreapproval
{
    static constexpr const char *choice_name = "AmuletRules_CreateTransferPreapproval";

    PaymentTransferContext context;
    std::vector<TransferInput> inputs;
    std::string receiver;
    std::string provider;
    std::string expiresAt;
    std::optional<std::string> expectedDso;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AmuletRules_CreateTransferPreapproval, context, inputs, receiver,
                                                provider, expiresAt, expectedDso)

} // namespace daml::model::choice
