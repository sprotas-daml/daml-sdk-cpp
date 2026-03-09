module;

#include <string>

export module daml.crypto:sign;

export namespace daml::crypto::sign
{
std::string sign_transaction(const std::string &transactionHash, const std::string &privateKey);
}
