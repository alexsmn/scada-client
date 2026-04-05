#pragma once

#include <filesystem>
#include <optional>

class Profile;
class WindowDefinition;

bool IsModus2(const WindowDefinition& definition, Profile& profile);

bool IsModusFilePath(const std::filesystem::path& path);

std::optional<std::filesystem::path> MakeModusFilePath(
    const std::filesystem::path& hyperlink_path,
    const std::filesystem::path& current_display_path);
