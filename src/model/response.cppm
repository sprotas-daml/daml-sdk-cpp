module;

#include <nlohmann/json.hpp>
#include <string>

export module daml.model:response;

export namespace daml::model::response
{
using json = nlohmann::json;

struct CreatedEvent
{
    std::string contractId;
    std::string templateId;
    json createArgument;
    std::string createdEventBlob;
    std::vector<json> interfaceViews;
    std::vector<std::string> witnessParties;
    std::vector<std::string> signatories;
    std::vector<std::string> observers;
    std::string createdAt;
    std::string packageName;
    std::string representativePackageId;
    bool acsDelta;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CreatedEvent, contractId, templateId, createArgument, createdEventBlob,
                                                interfaceViews, witnessParties, signatories, observers, createdAt,
                                                packageName, representativePackageId, acsDelta)

struct ExercisedEvent
{
    std::string contractId;
    std::string templateId;
    std::string interfaceId;
    std::string choice;
    nlohmann::json choiceArgument;
    std::vector<std::string> actingParties;
    bool consuming;
    std::vector<std::string> witnessParties;
    nlohmann::json exerciseResult;
    std::string packageName;
    std::vector<std::string> implementedInterfaces;
    bool acsDelta;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ExercisedEvent, contractId, templateId, choice, consuming,
                                                choiceArgument, exerciseResult)

struct EventWrapper
{
    std::optional<CreatedEvent> CreatedEvent;
    std::optional<ExercisedEvent> ExercisedEvent;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EventWrapper, CreatedEvent, ExercisedEvent)

struct TransactionValue
{
    std::string updateId;
    std::string effectiveAt;
    int64_t offset;
    std::vector<EventWrapper> events;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TransactionValue, updateId, effectiveAt, offset, events)

struct Transaction
{
    TransactionValue value;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Transaction, value)

struct UpdateValue
{
    std::optional<Transaction> Transaction;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UpdateValue, Transaction)

struct Update
{
    UpdateValue update;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Update, update)

} // namespace daml::model::response
