#pragma once

#include "base/base64.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"

inline std::string SaveBlob(base::StringPiece blob) {
  std::string text;
  base::Base64Encode(blob, &text);
  return text;
}

inline std::string RestoreBlob(base::StringPiece text) {
  auto trimmed_text =
      base::TrimString(text, base::kWhitespaceASCII, base::TRIM_ALL);
  std::string blob;
  base::Base64Decode(trimmed_text, &blob);
  return blob;
}
