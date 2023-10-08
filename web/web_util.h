#pragma once

#include <filesystem>
#include <string_view>

bool IsWebUrl(std::u16string_view str);
std::u16string MakeFileUrl(const std::filesystem::path& path);
