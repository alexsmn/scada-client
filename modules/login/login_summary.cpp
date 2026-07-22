#include "modules/login/login_summary.h"

namespace {

// Trims ASCII/Unicode spaces so a blank-but-not-empty combo entry (a stray
// space in a saved connection) counts as unknown rather than producing a
// dangling separator.
std::u16string_view Trim(std::u16string_view text) {
  const auto is_space = [](char16_t c) { return c == u' ' || c == u'\t'; };
  while (!text.empty() && is_space(text.front()))
    text.remove_prefix(1);
  while (!text.empty() && is_space(text.back()))
    text.remove_suffix(1);
  return text;
}

}  // namespace

std::u16string LoginConnectionSummary(std::u16string_view backend,
                                      std::u16string_view server) {
  const std::u16string_view trimmed_backend = Trim(backend);
  const std::u16string_view trimmed_server = Trim(server);

  if (trimmed_backend.empty())
    return std::u16string{trimmed_server};
  if (trimmed_server.empty())
    return std::u16string{trimmed_backend};

  std::u16string summary{trimmed_backend};
  summary += u" · ";
  summary += trimmed_server;
  return summary;
}
