module;

#include <intx/intx.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

export module daml.type:decimal;

using namespace intx;

export namespace daml::decimal
{
class decimal
{
  private:
    uint256 value_;

    static inline const uint256 SCALE = 10000000000_u256;
    static inline const uint256 HALF_SCALE = 5000000000_u256;

    struct raw_tag
    {
    };

    decimal(uint256 raw, raw_tag) : value_(raw)
    {
    }

  public:
    decimal() : value_(0)
    {
    }

    friend void to_json(nlohmann::json &j, const decimal &d)
    {
        j = d.to_string();
    }

    friend void from_json(const nlohmann::json &j, decimal &d)
    {
        if (j.is_string())
        {
            d = decimal(j.get<std::string>());
        }
        else
        {
            throw std::runtime_error("Expected string for decimal conversion");
        }
    }

    explicit decimal(std::string_view str)
    {
        uint256 res = 0;
        bool in_fraction = false;
        int fractional_digits = 0;

        for (char c : str)
        {
            if (c == '.')
            {
                if (in_fraction)
                {
                    throw std::invalid_argument("Multiple decimal points in string");
                }
                in_fraction = true;
                continue;
            }
            if (c < '0' || c > '9')
            {
                throw std::invalid_argument("Invalid character in decimal string");
            }

            res = (res * 10) + (c - '0');

            if (in_fraction)
            {
                fractional_digits++;
                if (fractional_digits > 10)
                {
                    throw std::invalid_argument("Exceeded maximum fractional precision (10)");
                }
            }
        }

        while (fractional_digits < 10)
        {
            res *= 10;
            fractional_digits++;
        }

        value_ = res;
    }

    [[nodiscard]] std::string to_string() const
    {
        std::string s = intx::to_string(value_);

        if (s.length() <= 10)
        {
            return "0." + std::string(10 - s.length(), '0') + s;
        }

        s.insert(s.length() - 10, ".");
        return s;
    }

    decimal operator+(const decimal &other) const
    {
        return decimal(value_ + other.value_, raw_tag{});
    }

    decimal operator-(const decimal &other) const
    {
        if (value_ < other.value_)
        {
            throw std::underflow_error("Fixed decimal underflow (result < 0)");
        }
        return decimal(value_ - other.value_, raw_tag{});
    }

    decimal operator*(const decimal &other) const
    {
        uint256 raw_product = value_ * other.value_;

        uint256 q = raw_product / SCALE;
        uint256 r = raw_product % SCALE;

        if (r > HALF_SCALE)
        {
            q += 1;
        }
        else if (r == HALF_SCALE)
        {
            if ((q & 1) != 0)
            {
                q += 1;
            }
        }

        return decimal(q, raw_tag{});
    }

    decimal operator/(const decimal &other) const
    {
        if (other.value_ == 0)
        {
            throw std::domain_error("Division by zero");
        }

        uint256 raw_dividend = value_ * SCALE;

        uint256 q = raw_dividend / other.value_;
        uint256 r = raw_dividend % other.value_;

        uint256 r2 = r * 2;

        if (r2 > other.value_)
        {
            q += 1;
        }
        else if (r2 == other.value_)
        {
            if ((q & 1) != 0)
            {
                q += 1;
            }
        }

        return decimal(q, raw_tag{});
    }

    auto operator<=>(const decimal &other) const = default;
};
} // namespace daml::decimal
