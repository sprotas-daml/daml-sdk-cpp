module;

#include <nlohmann/json.hpp>
#include <string>

export module daml.model:response;

import daml.type;
import daml.utils;
import :datatype;

export namespace daml::model::response
{
using json = nlohmann::json;
using namespace daml::utils::json_literals;
using namespace daml::model::datatype;

struct InterfaceView
{
    std::string interfaceId;
    json viewValue;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InterfaceView, interfaceId, viewValue)

struct CreatedEvent
{
    node_int_t offset;
    node_int_t nodeId;
    std::string contractId;
    std::string templateId;
    json createArgument;
    std::string createdEventBlob;
    std::vector<InterfaceView> interfaceViews;
    std::vector<std::string> witnessParties;
    std::vector<std::string> signatories;
    std::vector<std::string> observers;
    std::string createdAt;
    std::string packageName;
    std::string representativePackageId;
    bool acsDelta{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CreatedEvent, offset, nodeId, contractId, templateId, createArgument,
                                                createdEventBlob, interfaceViews, witnessParties, signatories,
                                                observers, createdAt, packageName, representativePackageId, acsDelta)

struct ExercisedEvent
{
    node_int_t offset;
    node_int_t nodeId;
    node_int_t lastDescendantNodeId;
    std::string contractId;
    std::string templateId;
    std::optional<std::string> interfaceId;
    std::string choice;
    nlohmann::json choiceArgument;
    std::vector<std::string> actingParties;
    bool consuming{};
    std::vector<std::string> witnessParties;
    nlohmann::json exerciseResult;
    std::string packageName;
    std::vector<std::string> implementedInterfaces;
    bool acsDelta{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ExercisedEvent, offset, nodeId, lastDescendantNodeId, contractId,
                                                templateId, interfaceId, choice, choiceArgument, actingParties,
                                                consuming, witnessParties, exerciseResult, packageName,
                                                implementedInterfaces, acsDelta)

struct ArchivedEvent
{
    node_int_t offset;
    node_int_t nodeId;
    std::string contractId;
    std::string templateId;
    std::vector<std::string> witnessParties;
    std::string packageName;
    std::vector<std::string> implementedInterfaces;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ArchivedEvent, offset, nodeId, contractId, templateId, witnessParties,
                                                packageName, implementedInterfaces)

struct EventWrapper
{
    std::optional<CreatedEvent> CreatedEvent;
    std::optional<ExercisedEvent> ExercisedEvent;
    std::optional<ArchivedEvent> ArchivedEvent;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EventWrapper, CreatedEvent, ExercisedEvent, ArchivedEvent)

struct Transaction
{
    std::string updateId;
    std::string effectiveAt;
    node_int_t offset = 0;
    std::vector<EventWrapper> events;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Transaction, updateId, effectiveAt, offset, events)

struct SubmitTransactionAndWaitTreeResponse
{
    Transaction transaction;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SubmitTransactionAndWaitTreeResponse, transaction)

struct Update
{
    std::string updateId;
    std::string effectiveAt;
    node_int_t offset = 0;
    std::vector<EventWrapper> events;
};

inline void to_json(nlohmann::json &j, const Update &u)
{
    j = jo{
        "update"_j =
            {
                "Transaction"_j =
                    {
                        "value"_j =
                            {
                                "updateId"_j = u.updateId,
                                "effectiveAt"_j = u.effectiveAt,
                                "offset"_j = u.offset,
                                "events"_j = u.events,
                            },
                    },
            },
    };
}

inline void from_json(const nlohmann::json &j, Update &u)
{
    const auto &value_node = j.at("update").at("Transaction").at("value");

    u.updateId = value_node.value("updateId", "");
    u.effectiveAt = value_node.value("effectiveAt", "");
    u.offset = value_node.value("offset", 0);

    if (value_node.contains("events") && !value_node["events"].is_null())
    {
        value_node.at("events").get_to(u.events);
    }
    else
    {
        u.events.clear();
    }
}

struct ActiveContract
{
    std::string synchronizerId;
    CreatedEvent createdEvent;
};

inline void from_json(const json &j, ActiveContract &a)
{
    const auto &jsActiveContract = j.at("contractEntry").at("JsActiveContract");

    jsActiveContract.at("synchronizerId").get_to(a.synchronizerId);
    jsActiveContract.at("createdEvent").get_to(a.createdEvent);
}

inline void to_json(json &j, const ActiveContract &a)
{
    j = jo{
        "workflowId"_j = "",
        "contractEntry"_jo =
            {
                "JsActiveContract"_j =
                    {
                        "synchronizerId"_j = a.synchronizerId,
                        "createdEvent"_j = a.createdEvent,
                        "reassignmentCounter"_j = 0,
                    },
            },
    };
}

struct Created
{
    CreatedEvent createdEvent;
    std::string synchronizerId;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Created, createdEvent, synchronizerId)

struct Archived
{
    ArchivedEvent archivedEvent;
    std::string synchronizerId;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Archived, archivedEvent, synchronizerId)

struct ContractEvents
{
    Created created;
    Archived archived;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ContractEvents, created, archived)

struct CostEstimation
{
    node_int_t confirmationRequestTrafficCostEstimation;
    node_int_t confirmationResponseTrafficCostEstimation;
    node_int_t totalTrafficCostEstimation;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CostEstimation, confirmationRequestTrafficCostEstimation,
                                                confirmationResponseTrafficCostEstimation, totalTrafficCostEstimation)

struct PreparedTransactionResponse
{
    std::string preparedTransaction;
    std::string preparedTransactionHash;
    std::string hashingSchemeVersion;
    CostEstimation costEstimation;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PreparedTransactionResponse, preparedTransaction,
                                                preparedTransactionHash, hashingSchemeVersion, costEstimation)

struct SubmissionResponse
{
    std::string updateId;
    node_int_t completionOffset;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SubmissionResponse, updateId, completionOffset)

struct ChoiceContext
{
    metadata_v1::ChoiceContext choiceContextData;
    std::vector<DisclosedContract> disclosedContracts;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ChoiceContext, choiceContextData, disclosedContracts)

struct TransferFactoryResponse
{
    std::string factoryId;
    std::string transferKind;
    ChoiceContext choiceContext;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TransferFactoryResponse, factoryId, transferKind, choiceContext)

} // namespace daml::model::response
