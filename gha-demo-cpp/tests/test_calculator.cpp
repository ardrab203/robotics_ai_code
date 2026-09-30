#include <gtest/gtest.h>
#include <stdexcept>
#include "calculator.h"

TEST(CalculatorTest, Add) {
    EXPECT_DOUBLE_EQ(add(1, 2), 3);
    EXPECT_DOUBLE_EQ(add(-1, 1), 0);
    EXPECT_DOUBLE_EQ(add(0, 0), 0);
    EXPECT_DOUBLE_EQ(add(2.5, 2.5), 5.0);
}

TEST(CalculatorTest, Subtract) {
    EXPECT_DOUBLE_EQ(subtract(5, 3), 2);
}

TEST(CalculatorTest, Multiply) {
    EXPECT_DOUBLE_EQ(multiply(4, 3), 12);
}

TEST(CalculatorTest, Divide) {
    EXPECT_DOUBLE_EQ(divide(10, 4), 2.5);
}

TEST(CalculatorTest, DivideByZeroThrows) {
    EXPECT_THROW(divide(1, 0), std::invalid_argument);
}