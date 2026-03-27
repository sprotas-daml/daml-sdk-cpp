module;

#include <nlohmann/json.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <spdlog/spdlog.h>

export module daml.api:submit;

import daml.model;
import daml.utils;
import daml.crypto;

import :client;

export namespace daml::api::request
{
using json = nlohmann::json;
using namespace daml::model::request;
using namespace daml::model::response;
using namespace daml::model::datatype;
using namespace daml::utils;

using int_t = std::uint64_t;
using str_ref_t = const std::string &;
using vec_str_ref_t = const std::vector<std::string> &;

struct TransactionPayload
{
    std::string user_id;
    std::vector<std::string> read_as;
    std::vector<std::string> act_as;
    std::string synchronizer_id;
    std::vector<DisclosedContract> disclosed_contracts;
    std::vector<CommandWrapper> commands;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TransactionPayload, user_id, read_as, act_as, synchronizer_id, disclosed_contracts, commands)

PreparedTransactionResponse prepare_transaction(str_ref_t token, const TransactionPayload &payload)
{
    PreparedTransaction req;
    req.commandId = uuid::uuid_v4();
    req.userId = payload.user_id;
    req.readAs = payload.read_as;
    req.actAs = payload.act_as;
    req.synchronizerId = payload.synchronizer_id;
    req.packageIdSelectionPreference = {};
    req.disclosedContracts = payload.disclosed_contracts;
    req.commands = payload.commands;
    req.verboseHashing = false;

    return client::ledger_post("v2/interactive-submission/prepare", token, req);
}

SubmissionResponse send_transaction(str_ref_t token, const TransactionPayload &payload)
{
    SubmitTransaction req;
    req.commandId = uuid::uuid_v4();
    req.submissionId = uuid::uuid_v4();
    req.userId = payload.user_id;
    req.readAs = payload.read_as;
    req.actAs = payload.act_as;
    req.synchronizerId = payload.synchronizer_id;
    req.packageIdSelectionPreference = {};
    req.disclosedContracts = payload.disclosed_contracts;
    req.commands = payload.commands;
    
    spdlog::info(nlohmann::json(req).dump(2));

    return client::ledger_post("v2/commands/submit-and-wait", token, req);
}

Signature build_ed25519_sig_for_party(str_ref_t party, str_ref_t private_key, str_ref_t hash)
{
    auto get_party_hash = [](const std::string &party) {
        if (const auto pos = party.find("::"); pos != std::string_view::npos)
        {
            return party.substr(pos + 2);
        }
        return party;
    };

    SignaturePayload party_sig = {
        .format = "SIGNATURE_FORMAT_CONCAT",
        .signingAlgorithmSpec = "SIGNING_ALGORITHM_SPEC_ED25519",
        .signedBy = get_party_hash(party),
        .signature = crypto::sign::sign_transaction(hash, private_key),
    };

    return {.party = party, .signatures = {party_sig}};
}

SubmissionResponse send_signed_transaction_one_party(str_ref_t token, str_ref_t user_id, str_ref_t party,
                                                     str_ref_t private_key, const TransactionPayload &payload)
{
    auto response = prepare_transaction(token, payload);

    PartySignatures sigs = {
        .signatures = {build_ed25519_sig_for_party(party, private_key, response.preparedTransactionHash)}};

    SubmitSignedTransaction req = {
        .userId = user_id,
        .submissionId = uuid::uuid_v4(),
        .hashingSchemeVersion = response.hashingSchemeVersion,
        .preparedTransaction = response.preparedTransaction,
        .partySignatures = sigs,
    };

    return client::ledger_post("v2/interactive-submission/executeAndWait", token, req);
}
}; // namespace daml::api::request
