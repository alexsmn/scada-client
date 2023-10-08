#pragma once

#include <filesystem>

std::filesystem::path GetPublicFilePath(const std::filesystem::path& path);
std::filesystem::path FullFilePathToPublic(const std::filesystem::path& path);
