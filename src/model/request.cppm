module;

#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <variant>
#include <vector>

export module daml.model:request;

import daml.type;
import :datatype;

export namespace daml::model::request
{
    using json = nlohmann::json;
    using int_t = int32_t;

    struct ExerciseCommand
    {
        std::string contractId;
        std::string choice;
        json choiceArgument;
        std::string templateId;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ExerciseCommand, contractId, choice, choiceArgument, templateId)

    struct CreateCommand
    {
        json createArguments;
        std::string templateId;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CreateCommand, createArguments, templateId)

    struct InterfaceFilter
    {
        struct InterfaceFilterValue
        {
            std::string interfaceId;
            bool includeCreatedEventBlob{};
            bool includeInterfaceView{};
        } value;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InterfaceFilter::InterfaceFilterValue, interfaceId,
                                                    includeCreatedEventBlob, includeInterfaceView)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InterfaceFilter, value)

    struct TemplateFilter
    {
        struct TemplateFilterValue
        {
            std::string templateId;
            bool includeCreatedEventBlob{};

        } value;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TemplateFilter::TemplateFilterValue, templateId,
                                                    includeCreatedEventBlob)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TemplateFilter, value)

    struct WildcardFilter
    {
        struct WildcardFilterValue
        {
            bool includeCreatedEventBlob;
        } value;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WildcardFilter::WildcardFilterValue, includeCreatedEventBlob)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WildcardFilter, value)

    using CommandWrapper = std::variant<ExerciseCommand, CreateCommand>;
    using FilterVariant = std::variant<InterfaceFilter, TemplateFilter, WildcardFilter>;
    
    struct BaseSubmit
    {
        std::string commandId;
        std::string userId;
        std::vector<std::string> readAs;
        std::vector<std::string> actAs;
        std::string synchronizerId;
        std::vector<std::string> packageIdSelectionPreference;
        std::vector<datatype::DisclosedContract> disclosedContracts;
        std::vector<CommandWrapper> commands;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BaseSubmit, commandId)

    // Submitting
    struct SubmitTransaction : public BaseSubmit
    {
        std::string submissionId;
    };
    NLOHMANN_DEFINE_DERIVED_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SubmitTransaction, BaseSubmit, submissionId)

    // Submitting with sign
    struct PreparedTransaction : public BaseSubmit
    {
        bool verboseHashing = false;
    };
    NLOHMANN_DEFINE_DERIVED_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PreparedTransaction, BaseSubmit, verboseHashing)

    struct SignaturePayload
    {
        std::string format = "SIGNATURE_FORMAT_CONCAT";
        std::string signingAlgorithmSpec = "SIGNING_ALGORITHM_SPEC_ED25519";
        std::string signedBy;
        std::string signature;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SignaturePayload, format, signingAlgorithmSpec, signedBy, signature)

    struct Signature
    {
        std::string party;
        std::vector<SignaturePayload> signatures;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Signature, party, signatures)

    struct PartySignatures
    {
        std::vector<Signature> signatures;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PartySignatures, signatures)

    struct SubmitSignedTransaction
    {
        std::string userId;
        std::string submissionId;
        std::string hashingSchemeVersion = "HASHING_SCHEME_VERSION_V2";
        std::string preparedTransaction;
        json deduplicationPeriod = {{"Empty", {}}};
        PartySignatures partySignatures;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SubmitSignedTransaction, userId, submissionId, hashingSchemeVersion,
                                                    preparedTransaction, deduplicationPeriod, partySignatures)

    struct FilterPayload
    {
        FilterVariant identifierFilter;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FilterPayload, identifierFilter)

    struct CumulativeFilters
    {
        std::vector<FilterPayload> cumulative;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CumulativeFilters, cumulative)

    struct BaseFilter
    {
        bool verbose;
        json filter;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BaseFilter, verbose, filter)

    struct FlatUpdatesRequest : public BaseFilter
    {
        int_t beginExclusive;
        int_t endInclusive;
    };
    NLOHMANN_DEFINE_DERIVED_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FlatUpdatesRequest, BaseFilter, beginExclusive, endInclusive)

    struct ActiveContractSetRequest : public BaseFilter
    {
        int_t activeAtOffset;
    };
    NLOHMANN_DEFINE_DERIVED_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ActiveContractSetRequest, BaseFilter, activeAtOffset)

    struct IncludeTransactions
    {
        std::string transactionShape = "TRANSACTION_SHAPE_ACS_DELTA";
        json eventFormat;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(IncludeTransactions, transactionShape, eventFormat)

    struct UpdateFormat
    {
        IncludeTransactions includeTransactions;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UpdateFormat, includeTransactions)

    struct UpdateByIdRequest
    {
        std::string updateId;
        UpdateFormat updateFormat;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UpdateByIdRequest, updateId, updateFormat)

    struct ContractByIdRequest
    {
        std::string contractId;
        json eventFormat;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ContractByIdRequest, contractId, eventFormat)

} // namespace daml::model::request


namespace nlohmann {
    template <>
    struct adl_serializer<daml::model::request::CommandWrapper> {
        static void to_json(json& j, const daml::model::request::CommandWrapper& v) {
            j = json::object();
            std::visit(
                [&](auto&& arg) {
                    using V = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<V, daml::model::request::ExerciseCommand>) {
                        j["ExerciseCommand"] = arg;
                    } else if constexpr (std::is_same_v<V, daml::model::request::CreateCommand>) {
                        j["CreateCommand"] = arg;
                    }
                },
                v);
        }

        static void from_json(const json& j, daml::model::request::CommandWrapper& v) {
            if (j.contains("ExerciseCommand")) {
                v = j.at("ExerciseCommand").get<daml::model::request::ExerciseCommand>();
            } else if (j.contains("CreateCommand")) {
                v = j.at("CreateCommand").get<daml::model::request::CreateCommand>();
            } else {
                throw std::runtime_error("Invalid json object: missing CommandWrapper tag");
            }
        }
    };

    template <>
    struct adl_serializer<daml::model::request::FilterVariant> {
        static void to_json(json& j, const daml::model::request::FilterVariant& v) {
            j = json::object();
            std::visit(
                [&](auto&& arg) {
                    using V = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<V, daml::model::request::InterfaceFilter>) {
                        j["InterfaceFilter"] = arg;
                    } else if constexpr (std::is_same_v<V, daml::model::request::TemplateFilter>) {
                        j["TemplateFilter"] = arg;
                    } else if constexpr (std::is_same_v<V, daml::model::request::WildcardFilter>) {
                        j["WildcardFilter"] = arg;
                    }
                },
                v);
        }

        static void from_json(const json& j, daml::model::request::FilterVariant& v) {
            if (j.contains("InterfaceFilter")) {
                v = j.at("InterfaceFilter").get<daml::model::request::InterfaceFilter>();
            } else if (j.contains("TemplateFilter")) {
                v = j.at("TemplateFilter").get<daml::model::request::TemplateFilter>();
            } else if (j.contains("WildcardFilter")) {
                v = j.at("WildcardFilter").get<daml::model::request::WildcardFilter>();
            } else {
                throw std::runtime_error("Invalid JSON: missing FilterVariant tag");
            }
        }
    };
} // namespace nlohmann
