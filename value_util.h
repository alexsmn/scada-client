#pragma once

#include "base/files/file_path.h"
#include "base/string_piece_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "json.h"

inline bool GetBool(const base::Value& value,
                    std::string_view key,
                    bool default_value = false) {
  if (!value.is_dict())
    return default_value;
  if (auto* k =
          value.FindKeyOfType(ToStringPiece(key), base::Value::Type::BOOLEAN))
    return k->GetBool();
  else
    return default_value;
}

inline int GetInt(const base::Value& value,
                  std::string_view key,
                  int default_value = 0) {
  if (!value.is_dict())
    return default_value;
  if (auto* k =
          value.FindKeyOfType(ToStringPiece(key), base::Value::Type::INTEGER))
    return k->GetInt();
  else
    return default_value;
}

inline std::string_view GetString(const base::Value& value,
                                  std::string_view key,
                                  std::string_view default_value = {}) {
  if (!value.is_dict())
    return default_value;
  if (auto* k =
          value.FindKeyOfType(ToStringPiece(key), base::Value::Type::STRING))
    return k->GetString();
  else
    return default_value;
}

inline std::wstring GetString16(const base::Value& value,
                                std::string_view key,
                                std::wstring_view default_value = {}) {
  if (!value.is_dict())
    return std::wstring{default_value};
  if (auto* k =
          value.FindKeyOfType(ToStringPiece(key), base::Value::Type::STRING))
    return base::UTF8ToUTF16(k->GetString());
  else
    return std::wstring{default_value};
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

bool SaveJsonToFile(const base::Value& data, const base::FilePath& path);

std::string SaveJsonToString(const base::Value& data);

std::unique_ptr<base::Value> LoadJsonFromFile(
    const base::FilePath& path,
    std::string* error_message = nullptr);

std::unique_ptr<base::Value> LoadJsonFromString(
    std::string_view contents,
    std::string* error_message = nullptr);
