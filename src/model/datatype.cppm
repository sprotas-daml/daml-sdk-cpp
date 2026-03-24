module;

#include <cstdint>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

export module daml.model:datatype;

import daml.type;

export namespace daml::model::datatype
{
using json = nlohmann::json;

struct DisclosedContract
{
    std::string templateId;
    std::string contractId;
    std::string createdEventBlob;
    std::string synchronizerId;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DisclosedContract, templateId, contractId, createdEventBlob, synchronizerId)

struct Round : public DisclosedContract
{
    node_int_t round_number{};

    std::chrono::system_clock::time_point opens_at;
    std::chrono::system_clock::time_point target_closes_at;

    std::optional<decimal::decimal> issuing_round_coefficient;
};
struct OpenAndIssuingRounds
{
    std::vector<Round> open_rounds;
    std::vector<Round> issuing_rounds;
};

struct RoundNumber
{
    node_int_t number;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RoundNumber, number)

using IssuingMiningRound = std::pair<RoundNumber, std::string>;

using ValidatorRight = std::pair<std::string, std::string>;

struct TransferContext
{
    std::string openMiningRound;
    std::vector<IssuingMiningRound> issuingMiningRounds;
    std::vector<ValidatorRight> validatorRights;
    std::optional<std::string> featuredAppRight;
};

struct PaymentTransferContext
{
    std::string amuletRules;
    TransferContext context;
};

struct AppRewardBeneficiary
{
    std::string beneficiary;
    decimal::decimal weight;
};

// data TransferOutput AmuletRules.daml
struct TransferOutput
{
    std::string receiver;
    decimal::decimal receiverFeeRatio;
    decimal::decimal amount;
    std::optional<json> lock; // TODO: complete type
};
// data TransferOutput AmuletRules.daml

// data TransferInput AmuletRules.daml
using ContractId = std::string;

struct ExtTransferInput
{
    std::nullptr_t dummyUnitField = nullptr;
    std::optional<ContractId> optInputValidatorFaucetCoupon;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ExtTransferInput, dummyUnitField, optInputValidatorFaucetCoupon)
};

struct BaseInput
{
    ContractId cid;
};

struct InputAppRewardCoupon : public BaseInput
{
};
struct InputValidatorRewardCoupon : public BaseInput
{
};
struct InputSvRewardCoupon : public BaseInput
{
};
struct InputAmulet : public BaseInput
{
};
struct InputValidatorLivenessActivityRecord : public BaseInput
{
};
struct InputUnclaimedActivityRecord : public BaseInput
{
};

using TransferInput =
    std::variant<InputAppRewardCoupon, InputValidatorRewardCoupon, InputSvRewardCoupon, InputAmulet, ExtTransferInput,
                 InputValidatorLivenessActivityRecord, InputUnclaimedActivityRecord>;

void to_json(nlohmann::json &j, const TransferInput &v)
{
    std::visit(
        [&](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, InputAppRewardCoupon>)
                j = {{"tag", "InputAppRewardCoupon"}, {"value", arg.cid}};
            else if constexpr (std::is_same_v<T, InputValidatorRewardCoupon>)
                j = {{"tag", "InputValidatorRewardCoupon"}, {"value", arg.cid}};
            else if constexpr (std::is_same_v<T, InputSvRewardCoupon>)
                j = {{"tag", "InputSvRewardCoupon"}, {"value", arg.cid}};
            else if constexpr (std::is_same_v<T, InputAmulet>)
                j = {{"tag", "InputAmulet"}, {"value", arg.cid}};
            else if constexpr (std::is_same_v<T, ExtTransferInput>)
                j = {{"tag", "ExtTransferInput"}, {"value", arg}}; // Здесь value будет объектом
            else if constexpr (std::is_same_v<T, InputValidatorLivenessActivityRecord>)
                j = {{"tag", "InputValidatorLivenessActivityRecord"}, {"value", arg.cid}};
            else if constexpr (std::is_same_v<T, InputUnclaimedActivityRecord>)
                j = {{"tag", "InputUnclaimedActivityRecord"}, {"value", arg.cid}};
        },
        v);
}

void from_json(const nlohmann::json &j, TransferInput &v)
{
    std::string tag = j.at("tag").get<std::string>();
    const auto &val = j.at("value");

    if (tag == "InputAppRewardCoupon")
        v = InputAppRewardCoupon{val.get<ContractId>()};
    else if (tag == "InputValidatorRewardCoupon")
        v = InputValidatorRewardCoupon{val.get<ContractId>()};
    else if (tag == "InputSvRewardCoupon")
        v = InputSvRewardCoupon{val.get<ContractId>()};
    else if (tag == "InputAmulet")
        v = InputAmulet{val.get<ContractId>()};
    else if (tag == "ExtTransferInput")
        v = val.get<ExtTransferInput>();
    else if (tag == "InputValidatorLivenessActivityRecord")
        v = InputValidatorLivenessActivityRecord{val.get<ContractId>()};
    else if (tag == "InputUnclaimedActivityRecord")
        v = InputUnclaimedActivityRecord{val.get<ContractId>()};
}
// data TransferInput AmuletRules.daml

// data Transfer AmuletRules.daml
struct Transfer
{
    std::string sender;
    std::string provider;
    std::vector<TransferInput> inputs;
    std::vector<TransferOutput> outputs;
    std::optional<AppRewardBeneficiary> beneficiaries;
};

// data Transfer AmuletRules.daml

// data HoldingView HoldingV1.daml
struct Lock
{ // TODO: add more info about Lock
    std::vector<std::string> holders;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Lock, holders)

struct InstrumentId
{
    std::string admin;
    std::string id;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InstrumentId, admin, id)

struct HoldingView
{
    std::string owner;
    decimal::decimal amount;
    InstrumentId instrumentId;
    std::optional<Lock> lock;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HoldingView, owner, amount, instrumentId, lock)
// data HoldingView HoldingV1.daml

namespace metadata_v1
{
struct AnyValue
{
    std::string tag;
    json value;

    static AnyValue Text(const std::string &t)
    {
        return {"AV_Text", t};
    }
    static AnyValue Int(int64_t i)
    {
        return {"AV_Int", i};
    }
    static AnyValue Decimal(const std::string &d)
    {
        return {"AV_Decimal", d};
    }
    static AnyValue Bool(bool b)
    {
        return {"AV_Bool", b};
    }
    static AnyValue Party(const std::string &p)
    {
        return {"AV_Party", p};
    }
    static AnyValue ContractId(const std::string &cid)
    {
        return {"AV_ContractId", cid};
    }

    static AnyValue List(const std::vector<AnyValue> &l);
    static AnyValue Map(const std::unordered_map<std::string, AnyValue> &m);
};

inline void to_json(json &j, const AnyValue &av)
{
    j = json{{"tag", av.tag}, {"value", av.value}};
}

inline void from_json(const json &j, AnyValue &av)
{
    j.at("tag").get_to(av.tag);
    j.at("value").get_to(av.value);
}

inline AnyValue AnyValue::List(const std::vector<AnyValue> &l)
{
    return {"AV_List", l};
}

inline AnyValue AnyValue::Map(const std::unordered_map<std::string, AnyValue> &m)
{
    return {"AV_Map", m};
}

struct ChoiceContext
{
    std::unordered_map<std::string, AnyValue> values;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ChoiceContext, values)

struct Metadata
{
    std::unordered_map<std::string, std::string> values;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Metadata, values)
struct ExtraArgs
{
    ChoiceContext context;
    Metadata meta;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ExtraArgs, context, meta)
} // namespace metadata_v1

namespace transfer_instruction_v1
{

struct Transfer
{
    std::string sender;
    std::string receiver;
    decimal::decimal amount;
    InstrumentId instrumentId;
    std::string requestedAt;
    std::string executeBefore;
    std::vector<std::string> inputHoldingCids;
    metadata_v1::Metadata meta;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Transfer, sender, receiver, amount, instrumentId, requestedAt,
                                                executeBefore, inputHoldingCids, meta)

struct TransferFactory_Transfer
{
    std::string expectedAdmin;
    Transfer transfer;
    metadata_v1::ExtraArgs extraArgs;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TransferFactory_Transfer, expectedAdmin, transfer, extraArgs)
} // namespace transfer_instruction_v1
} // namespace daml::model::datatype
