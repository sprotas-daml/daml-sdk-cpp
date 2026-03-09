module;

#include <string>

export module daml.utils:sign;

export namespace daml::sign {
std::string signTransaction(const std::string &transactionHash, const std::string &privateKey);
}
