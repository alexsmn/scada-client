#pragma once

#include <QVariantList>
#include <oaidl.h>
#include <optional>
#include <span>

inline std::optional<QVariant> ToQVariant(const VARIANT& v) {
  switch (v.vt) {
    case VT_BSTR:
      return QVariant{QString::fromWCharArray(v.bstrVal)};
    default:
      // TODO: Add support for all types.
      assert(false);
      return std::nullopt;
  }
}

inline std::optional<QVariantList> ToQVariantList(
    std::span<const VARIANT> win_variants) {
  QVariantList result;
  result.reserve(win_variants.size());
  for (const auto& win_variant : win_variants) {
    if (auto v = ToQVariant(win_variant)) {
      result.push_back(std::move(*v));
    } else {
      return std::nullopt;
    }
  }
  return result;
}