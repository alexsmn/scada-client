#include "base/value_util.h"

#include "base/strings/utf_string_conversions.h"

std::u16string GetString16(const base::Value& value,
                           std::string_view key,
                           std::u16string_view default_value) {
  if (!value.is_dict())
    return std::u16string{default_value};
  if (auto* k =
          value.FindKeyOfType(AsStringPiece(key), base::Value::Type::STRING))
    return base::UTF8ToUTF16(k->GetString());
  else
    return std::u16string{default_value};
}
