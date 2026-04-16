#pragma once

#include <exception>
#include <optional>
#include <string>

std::optional<std::string> DescribeStartupException(
    std::exception_ptr exception);
