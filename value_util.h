#pragma once

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/string_piece_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"

#include <optional>
#include <string>

template <class T>
std::optional<T> FromJson(const base::Value& value);

inline bool GetBool(const base::Value& value,
                    std::string_view key,
                    bool default = false) {
  if (!value.is_dict())
    return nullptr;
  if (auto* k =
          value.FindKeyOfType(ToStringPiece(key), base::Value::Type::BOOLEAN))
    return k->GetBool();
  else
    return default;
}

inline int GetInt(const base::Value& value,
                  std::string_view key,
                  int default = 0) {
  if (!value.is_dict())
    return default;
  if (auto* k =
          value.FindKeyOfType(ToStringPiece(key), base::Value::Type::INTEGER))
    return k->GetInt();
  else
    return default;
}

inline std::string_view GetString(const base::Value& value,
                                  std::string_view key,
                                  std::string_view default = {}) {
  if (!value.is_dict())
    return default;
  if (auto* k =
          value.FindKeyOfType(ToStringPiece(key), base::Value::Type::STRING))
    return k->GetString();
  else
    return default;
}

inline std::wstring GetString16(const base::Value& value,
                                std::string_view key,
                                std::wstring_view default = {}) {
  if (!value.is_dict())
    return std::wstring{default};
  if (auto* k =
          value.FindKeyOfType(ToStringPiece(key), base::Value::Type::STRING))
    return base::UTF8ToUTF16(k->GetString());
  else
    return std::wstring{default};
}

inline const base::Value* GetDict(const base::Value& value,
                                  std::string_view key) {
  if (!value.is_dict())
    return nullptr;
  return value.FindKeyOfType(ToStringPiece(key), base::Value::Type::DICTIONARY);
}

inline const base::Value::ListStorage* GetList(const base::Value& value,
                                               std::string_view key) {
  if (!value.is_dict())
    return nullptr;
  if (auto* k =
          value.FindKeyOfType(ToStringPiece(key), base::Value::Type::LIST))
    return &k->GetList();
  else
    return nullptr;
}

template <class T>
inline std::optional<T> GetKey(const base::Value& dict, std::string_view key);

template <>
inline std::optional<int> GetKey(const base::Value& dict,
                                 std::string_view key) {
  const auto* k =
      dict.FindKeyOfType(ToStringPiece(key), base::Value::Type::INTEGER);
  return k ? std::make_optional(k->GetInt()) : std::nullopt;
}

inline void SetKey(base::Value& dict, std::string_view key, bool value) {
  dict.SetKey(ToStringPiece(key), base::Value{value});
}

inline void SetKey(base::Value& dict, std::string_view key, int value) {
  dict.SetKey(ToStringPiece(key), base::Value{value});
}

inline void SetKey(base::Value& dict, std::string_view key, const char* value) {
  dict.SetKey(ToStringPiece(key), base::Value{value});
}

inline void SetKey(base::Value& dict,
                   std::string_view key,
                   const wchar_t* value) {
  dict.SetKey(ToStringPiece(key), base::Value{value});
}

inline void SetKey(base::Value& dict,
                   std::string_view key,
                   std::string_view value) {
  dict.SetKey(ToStringPiece(key), base::Value{ToStringPiece(value)});
}

inline void SetKey(base::Value& dict,
                   std::string_view key,
                   std::wstring_view value) {
  dict.SetKey(ToStringPiece(key), base::Value{ToStringPiece(value)});
}

inline void SetKey(base::Value& dict,
                   std::string_view key,
                   base::Value::ListStorage&& value) {
  dict.SetKey(ToStringPiece(key), base::Value{std::move(value)});
}

inline bool SaveJson(const base::Value& data, const base::FilePath& path) {
  JSONFileValueSerializer serializer{path};
  return serializer.Serialize(data);
}

inline std::unique_ptr<base::Value> LoadJson(
    const base::FilePath& path,
    std::string* error_message = nullptr) {
  JSONFileValueDeserializer deserializer{path,
                                         base::JSON_ALLOW_TRAILING_COMMAS};
  return deserializer.Deserialize(nullptr, error_message);
}
