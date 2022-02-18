#include "client_utils.h"

#include "base/string_piece_util.h"
#include "base/strings/utf_string_conversions.h"
#include "translation.h"

std::u16string Translate(std::string_view text) {
  return base::UTF8ToUTF16(AsStringPiece(text));
}

std::u16string FormatHostName(std::string_view host_name) {
  return base::UTF8ToUTF16(AsStringPiece(host_name));
}
