module;

#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

export module daml.model:choice;

import daml.decimal;
import :datatype;

export namespace daml::model::choice
{
using json = nlohmann::json;

struct SubscriptionProxy_CollectPartOfSubscriptionPaymentWithAppMarker
{
    static constexpr const char *choice_name = "SubscriptionProxy_CollectPartOfSubscriptionPaymentWithAppMarker";

    std::string requester;
    std::string pixSubscriptionCid;
    std::vector<std::string> amulets;
    datatype::PaymentTransferContext paymentTransferContext;
    std::string featuredAppRightCid;
    std::vector<std::string> additionalObservers;
};

struct PixSubscription_Cancel
{
    static constexpr const char *choice_name = "PixSubscription_Cancel";

    std::string requester;
};

struct AmuletRules_Transfer
{
    static constexpr const char *choice_name = "AmuletRules_Transfer";

    datatype::Transfer transfer;
    datatype::TransferContext context;
    std::string expectedDso;
};
} // namespace daml_sdk::model::choice
