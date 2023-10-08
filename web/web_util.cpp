#include "web/web_util.h"

#include "base/string_piece_util.h"
#include "base/strings/string_util.h"

namespace {

const char16_t kHttpPrefix[] = u"http://";
const char16_t kHttpsPrefix[] = u"https://";
const char16_t kFilePrefix[] = u"file://";

}  // namespace

bool IsWebUrl(std::u16string_view str) {
  return base::StartsWith(AsStringPiece(str), kHttpPrefix,
                          base::CompareCase::SENSITIVE) ||
         base::StartsWith(AsStringPiece(str), kHttpsPrefix,
                          base::CompareCase::SENSITIVE);
}

std::u16string MakeFileUrl(const std::filesystem::path& path) {
  return kFilePrefix + path.u16string();
}
