module;
#include <random>
#include <spdlog/spdlog.h>
#include <string>
export module daml.utils:uuid;

export namespace daml::utils::uuid
{
std::string uuid_v4()
{
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> byte_dist(0, 255);

    std::array<unsigned char, 16> bytes{};
    for (unsigned char &byte : bytes)
    {
        byte = static_cast<unsigned char>(byte_dist(rng));
    }

    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

    static constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(36);

    for (size_t index = 0; index < bytes.size(); ++index)
    {
        if (index == 4 || index == 6 || index == 8 || index == 10)
        {
            out.push_back('-');
        }

        const unsigned char byte = bytes[index];
        out.push_back(hex[(byte >> 4) & 0x0F]);
        out.push_back(hex[byte & 0x0F]);
    }

    return out;
}
} // namespace daml::utils::uuid
