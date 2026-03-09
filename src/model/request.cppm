module;

#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

export module daml.model:request;

import daml.type;
import :datatype;

export namespace daml::model::request
{
using json = nlohmann::json;

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

struct CommandWrapper
{
    std::optional<ExerciseCommand> ExerciseCommand;
    std::optional<CreateCommand> CreateCommand;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CommandWrapper, ExerciseCommand, CreateCommand)

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
} // namespace daml_sdk::model::request
