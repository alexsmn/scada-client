#include "base/json.h"

#include "base/file_path_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/json/json_string_value_serializer.h"

bool SaveJsonToFile(const base::Value& data,
                    const std::filesystem::path& path) {
  JSONFileValueSerializer serializer{AsFilePath(path)};
  return serializer.Serialize(data);
}

std::string SaveJsonToString(const base::Value& data) {
  std::string json_string;
  JSONStringValueSerializer serializer{&json_string};
  if (!serializer.Serialize(data))
    throw new std::runtime_error("Cannot serialize the value to JSON");
  return json_string;
}

std::optional<base::Value> LoadJsonFromFile(const std::filesystem::path& path,
                                            std::string* error_message) {
  JSONFileValueDeserializer deserializer{AsFilePath(path),
                                         base::JSON_ALLOW_TRAILING_COMMAS};
  auto value_ptr = deserializer.Deserialize(nullptr, error_message);
  return value_ptr ? std::make_optional(std::move(*value_ptr)) : std::nullopt;
}

std::optional<base::Value> LoadJsonFromString(std::string_view contents,
                                              std::string* error_message) {
  JSONStringValueDeserializer deserializer{contents,
                                           base::JSON_ALLOW_TRAILING_COMMAS};
  auto value_ptr = deserializer.Deserialize(nullptr, error_message);
  return value_ptr ? std::make_optional(std::move(*value_ptr)) : std::nullopt;
}
