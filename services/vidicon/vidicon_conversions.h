#pragma once

#include "base/string_piece_util.h"
#include "base/strings/string_number_conversions.h"
#include "scada/data_value.h"
#include "services/vidicon/data_point_address.h"

#include <optional>

namespace vidicon {

inline std::optional<DataPointAddress> ParseDataPointAddress(
    std::wstring_view str) {
  if (str.starts_with(L"AE:")) {
    return std::nullopt;
  }

  if (str.starts_with(L"CF:")) {
    str = str.substr(3);
    unsigned vidicon_id = 0;
    if (!base::StringToUint(AsStringPiece(str), &vidicon_id)) {
      return std::nullopt;
    }
    return DataPointAddress{.vidicon_id = vidicon_id};
  }

  if (str.starts_with(L"DA:")) {
    str = str.substr(3);
  }

  return DataPointAddress{.opc_address = std::wstring{str}};
}

}  // namespace vidicon