#include "app/startup_exception.h"

#include "app/client_application.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(StartupExceptionTest, SuppressesCanceledLogin) {
  EXPECT_EQ(DescribeStartupException(
                std::make_exception_ptr(LoginCanceled{})),
            std::nullopt);
}

TEST(StartupExceptionTest, DescribesStandardException) {
  EXPECT_EQ(
      DescribeStartupException(
          std::make_exception_ptr(std::runtime_error{"Login failed"})),
      "Login failed");
}

TEST(StartupExceptionTest, DescribesUnknownException) {
  EXPECT_EQ(DescribeStartupException(std::make_exception_ptr(42)),
            "unknown exception");
}
