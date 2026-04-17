#pragma once

#include <exception>
#include <optional>
#include <string>

std::optional<std::string> DescribeStartupException(
    std::exception_ptr exception);

std::optional<std::string> GetStartupErrorMessage(
    std::exception_ptr exception);
