module;

#include <vector>

#include <fmt/format.h>
#include <openssl/evp.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

module daml.utils;

namespace daml::sign
{

std::vector<unsigned char> hexToBytes(const std::string &hex)
{
    std::vector<unsigned char> bytes;
    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);

        auto byte = static_cast<unsigned char>(std::stol(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::vector<unsigned char> base64Decode(const std::string &input)
{
    if (input.empty())
        return {};

    std::vector<unsigned char> dest;
    dest.resize(3 * (input.size() / 4));

    int len = EVP_DecodeBlock(dest.data(), reinterpret_cast<const unsigned char *>(input.data()), input.size());

    if (len < 0)
    {
        throw std::runtime_error("OpenSSL Base64 decode failed");
    }

    // Adjust size: EVP_DecodeBlock translates Base64 padding ('=') into '\x00'.
    int padding = 0;
    if (input.size() > 0 && input[input.size() - 1] == '=')
        padding++;
    if (input.size() > 1 && input[input.size() - 2] == '=')
        padding++;

    dest.resize(len - padding);
    return dest;
}

std::string base64Encode(const std::vector<unsigned char> &data)
{
  if (data.empty()) return "";
  
    std::string dest;
    dest.resize(4 * ((data.size() + 2) / 3)); 
  
    int len = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(dest.data()), 
                              data.data(), 
                              data.size());
    dest.resize(len);
    return dest;
}

std::vector<unsigned char> signWithED25519RawKey(const std::vector<unsigned char> &rawKeyBytes,
                                                 const std::vector<unsigned char> &rawData)
{
    if (rawKeyBytes.size() != 64)
    {
        spdlog::error("RawKey wrong size: Expected 64 bytes, got {}", rawKeyBytes.size());
        return {};
    }

    // 1. Load Key: We only use the first 32 bytes (Private Seed)
    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, rawKeyBytes.data(), 32);
    if (!pkey)
    {
        spdlog::error("Failed creating EVP_PKEY from raw private key.");
        return {};
    }

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx)
    {
        spdlog::error("Failed to create EVP_MD_CTX.");
        EVP_PKEY_free(pkey);
        return {};
    }

    // 2. Init
    if (EVP_DigestSignInit(md_ctx, nullptr, nullptr, nullptr, pkey) <= 0)
    {
        spdlog::error("EVP_DigestSignInit failed.");
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return {};
    }

    // 3. Calculate Size
    size_t sig_len = 0;
    if (EVP_DigestSign(md_ctx, nullptr, &sig_len, rawData.data(), rawData.size()) <= 0)
    {
        spdlog::error("EVP_DigestSign failed while calculating signature size.");
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return {};
    }

    // 4. Sign
    std::vector<unsigned char> signature(sig_len);
    if (EVP_DigestSign(md_ctx, signature.data(), &sig_len, rawData.data(), rawData.size()) <= 0)
    {
        spdlog::error("EVP_DigestSign failed while performing signature.");
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return {};
    }

    signature.resize(sig_len);

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);

    return signature;
}

std::string signTransaction(const std::string &transactionHash, const std::string &privateKey)
{
    spdlog::info("Signing transaction hash: {}", transactionHash);
    if (privateKey.empty())
    {
        throw std::runtime_error("private key is empty, cannot sign transaction.");
    }

    std::vector<unsigned char> rawHash = base64Decode(transactionHash);

    std::vector<unsigned char> keyBytes = hexToBytes(privateKey);

    std::vector<unsigned char> rawSignature = signWithED25519RawKey(keyBytes, rawHash);

    if (rawSignature.empty())
    {
        throw std::runtime_error("failed to sign transaction, raw signature is empty.");
    }

    std::string signatureBase64 = base64Encode(rawSignature);
    spdlog::info("Successfully signed transaction. Signature: {}", signatureBase64);

    return signatureBase64;
}
} // namespace sign
