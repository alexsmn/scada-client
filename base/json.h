#pragma once

#include "base/values.h"

#include <optional>
#include <string>

namespace std::filesystem {
class path;
}

template <class T>
std::optional<T> FromJson(const base::Value& value);

std::optional<base::Value> LoadJsonFromFile(
    const std::filesystem::path& path,
    std::string* error_message = nullptr);

std::optional<base::Value> LoadJsonFromString(
    std::string_view contents,
    std::string* error_message = nullptr);

bool SaveJsonToFile(const base::Value& data, const std::filesystem::path& path);

std::string SaveJsonToString(const base::Value& data);
