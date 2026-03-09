#include <gtest/gtest.h>

import daml.type;

using namespace daml::decimal;

TEST(FixedDecimalTest, ParsingValidStrings) {
    // Standard fractional number
    decimal a("1.5");
    EXPECT_EQ(a.to_string(), "1.5000000000");

    // Integer without decimal point
    decimal b("42");
    EXPECT_EQ(b.to_string(), "42.0000000000");

    // Tiny fraction
    decimal c("0.0000000001");
    EXPECT_EQ(c.to_string(), "0.0000000001");
}

TEST(FixedDecimalTest, ParsingInvalidStrings) {
    // Multiple dots
    EXPECT_THROW(decimal("1.5.5"), std::invalid_argument);
    // Invalid characters
    EXPECT_THROW(decimal("abc"), std::invalid_argument);
    // Exceeds max precision (10 decimals)
    EXPECT_THROW(decimal("1.12345678901"), std::invalid_argument);
}

TEST(FixedDecimalTest, AdditionAndSubtraction) {
    decimal a("100.5");
    decimal b("50.5");
    
    decimal sum = a + b;
    EXPECT_EQ(sum.to_string(), "151.0000000000");

    decimal diff = a - b;
    EXPECT_EQ(diff.to_string(), "50.0000000000");

    // Prevent negative balances
    EXPECT_THROW(b - a, std::underflow_error);
}

TEST(FixedDecimalTest, MultiplicationBankersRounding) {
    decimal a("1.0000000001"); 
    decimal half("0.5"); 
    
    // Remainder is exactly half, round down to nearest even
    decimal even_result = a * half;
    EXPECT_EQ(even_result.to_string(), "0.5000000000");

    decimal b("1.0000000003");

    // Remainder is exactly half, round up to nearest even
    decimal odd_result = b * half;
    EXPECT_EQ(odd_result.to_string(), "0.5000000002");

    decimal more_than_half("0.6");
    
    // Remainder > half, strictly round up
    decimal up_result = a * more_than_half;
    EXPECT_EQ(up_result.to_string(), "0.6000000001");
}

TEST(FixedDecimalTest, Comparisons) {
    // Test C++20 spaceship operator (<=>) results
    decimal a("10.0");
    decimal b("20.0");
    decimal c("10.0");

    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a != b);
}