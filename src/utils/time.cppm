module;

#include <chrono>
#include <string>

export module daml.utils:time;

export namespace daml::utils::time
{
using namespace std::chrono;

system_clock::time_point parse_utc_iso8601(const std::string &time)
{
    if (time.size() < 20)
        throw std::runtime_error("invalid time string");

    system_clock::time_point tp{0s};
    tp = sys_days(year(stoi(time.substr(0, 4))) / month(stoi(time.substr(5, 2))) / day(stoi(time.substr(8, 2))));
    tp += hours(stoi(time.substr(11, 2)));
    tp += minutes(stoi(time.substr(14, 2)));
    tp += seconds(stoi(time.substr(17, 2)));
    if (time[19] == '.')
    {
        constexpr size_t start = 20;
        const size_t end = time.find('Z', start);
        std::string frac = time.substr(start, end - start);
        while (frac.size() < 6)
            frac += '0';
        if (frac.size() > 6)
            frac.resize(6);
        tp += microseconds(stol(frac));
    }

    return tp;
}
} // namespace daml::utils::time
