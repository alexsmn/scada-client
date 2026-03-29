#pragma once

#include <boost/json.hpp>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

std::optional<boost::json::value> LoadJsonFromFile(
    const std::filesystem::path& path,
    std::string* error_message = nullptr);

std::optional<boost::json::value> LoadJsonFromString(
    std::string_view contents,
    std::string* error_message = nullptr);

bool SaveJsonToFile(const boost::json::value& data,
                    const std::filesystem::path& path);

std::string SaveJsonToString(const boost::json::value& data);

template <class T>
std::optional<T> FromJson(const boost::json::value& value);
