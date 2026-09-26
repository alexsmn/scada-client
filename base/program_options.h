#pragma once

#include <boost/program_options.hpp>

#include <string>
#include <string_view>

namespace client {

void InitProgramOptions(int argc, char* argv[]);
bool HasOption(std::string_view name);
std::string GetOptionValue(std::string_view name);

}  // namespace client
