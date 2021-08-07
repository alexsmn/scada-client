#include "value_util.h"

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/json/json_string_value_serializer.h"

bool SaveJsonToFile(const base::Value& data, const base::FilePath& path) {
  JSONFileValueSerializer serializer{path};
  return serializer.Serialize(data);
}

std::string SaveJsonToString(const base::Value& data) {
  std::string json_string;
  JSONStringValueSerializer serializer{&json_string};
  if (!serializer.Serialize(data))
    throw new std::runtime_error("Cannot serialize the value to JSON");
  return json_string;
}

std::unique_ptr<base::Value> LoadJsonFromFile(const base::FilePath& path,
                                              std::string* error_message) {
  JSONFileValueDeserializer deserializer{path,
                                         base::JSON_ALLOW_TRAILING_COMMAS};
  return deserializer.Deserialize(nullptr, error_message);
}

std::unique_ptr<base::Value> LoadJsonFromString(std::string_view contents,
                                                std::string* error_message) {
  JSONStringValueDeserializer deserializer{ToStringPiece(contents),
                                           base::JSON_ALLOW_TRAILING_COMMAS};
  return deserializer.Deserialize(nullptr, error_message);
}
