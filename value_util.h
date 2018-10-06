#pragma once

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"

inline bool GetBool(const base::Value& value,
                    base::StringPiece key,
                    bool default = false) {
  if (auto* k = value.FindKeyOfType(key, base::Value::Type::BOOLEAN))
    return k->GetBool();
  else
    return default;
}

inline int GetInt(const base::Value& value,
                  base::StringPiece key,
                  int default = 0) {
  if (auto* k = value.FindKeyOfType(key, base::Value::Type::INTEGER))
    return k->GetInt();
  else
    return default;
}

inline base::StringPiece GetString(const base::Value& value,
                                   base::StringPiece key,
                                   base::StringPiece default = {}) {
  if (auto* k = value.FindKeyOfType(key, base::Value::Type::STRING))
    return k->GetString();
  else
    return default;
}

inline base::string16 GetString16(const base::Value& value,
                                  base::StringPiece key,
                                  base::StringPiece16 default = {}) {
  if (auto* k = value.FindKeyOfType(key, base::Value::Type::STRING))
    return base::UTF8ToUTF16(k->GetString());
  else
    return default.as_string();
}

inline const base::Value* GetDict(const base::Value& value,
                                  base::StringPiece key) {
  return value.FindKeyOfType(key, base::Value::Type::DICTIONARY);
}

inline const base::Value::ListStorage* GetList(const base::Value& value,
                                               base::StringPiece key) {
  if (auto* k = value.FindKeyOfType(key, base::Value::Type::LIST))
    return &k->GetList();
  else
    return nullptr;
}

inline const base::Value::BlobStorage* GetBlob(const base::Value& value,
                                               base::StringPiece key) {
  if (auto* k = value.FindKeyOfType(key, base::Value::Type::BINARY))
    return &k->GetBlob();
  else
    return nullptr;
}

inline void SetKey(base::Value& dict, base::StringPiece key, bool value) {
  dict.SetKey(key, base::Value{value});
}

inline void SetKey(base::Value& dict, base::StringPiece key, int value) {
  dict.SetKey(key, base::Value{value});
}

inline void SetKey(base::Value& dict,
                   base::StringPiece key,
                   const char* value) {
  dict.SetKey(key, base::Value{value});
}

inline void SetKey(base::Value& dict,
                   base::StringPiece key,
                   base::StringPiece value) {
  dict.SetKey(key, base::Value{value});
}

inline void SetKey(base::Value& dict,
                   base::StringPiece key,
                   base::StringPiece16 value) {
  dict.SetKey(key, base::Value{base::UTF16ToUTF8(value)});
}

inline void SetKey(base::Value& dict,
                   base::StringPiece key,
                   base::Value::ListStorage&& value) {
  dict.SetKey(key, base::Value{std::move(value)});
}

inline void SetKey(base::Value& dict,
                   base::StringPiece key,
                   base::Value::BlobStorage&& value) {
  dict.SetKey(key, base::Value{std::move(value)});
}

inline void SaveJson(const base::Value& data, const base::FilePath& path) {
  JSONFileValueSerializer serializer{path};
  serializer.Serialize(data);
}

inline std::unique_ptr<base::Value> LoadJson(
    const base::FilePath& path,
    std::string* error_message = nullptr) {
  JSONFileValueDeserializer deserializer{path,
                                         base::JSON_ALLOW_TRAILING_COMMAS};
  return deserializer.Deserialize(nullptr, error_message);
}
