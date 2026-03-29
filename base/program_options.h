#pragma once

#include <boost/program_options.hpp>

namespace client {

void InitProgramOptions();
bool HasOption(std::string_view name);
std::string GetOptionValue(std::string_view name);

}  // namespace client
