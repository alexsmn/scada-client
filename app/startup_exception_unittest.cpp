#include "app/startup_exception.h"

#include "app/login_canceled.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(StartupExceptionTest, SuppressesCanceledLogin) {
  EXPECT_EQ(DescribeStartupException(
                std::make_exception_ptr(LoginCanceled{})),
            std::nullopt);
}

TEST(StartupExceptionTest, SuppressesStartupErrorMessageForCanceledLogin) {
  EXPECT_EQ(GetStartupErrorMessage(std::make_exception_ptr(LoginCanceled{})),
            std::nullopt);
}

TEST(StartupExceptionTest, DescribesStandardException) {
  EXPECT_EQ(
      DescribeStartupException(
          std::make_exception_ptr(std::runtime_error{"Login failed"})),
      "Login failed");
}

TEST(StartupExceptionTest, FormatsStartupErrorMessage) {
  EXPECT_EQ(GetStartupErrorMessage(
                std::make_exception_ptr(std::runtime_error{"Login failed"})),
            "main() exception: Login failed");
}

TEST(StartupExceptionTest, DescribesUnknownException) {
  EXPECT_EQ(DescribeStartupException(std::make_exception_ptr(42)),
            "unknown exception");
}

TEST(StartupExceptionTest, FormatsUnknownStartupErrorMessage) {
  EXPECT_EQ(GetStartupErrorMessage(std::make_exception_ptr(42)),
            "main() exception: unknown exception");
}
