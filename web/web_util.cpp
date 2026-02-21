#include "web/web_util.h"

namespace {

const char16_t kFilePrefix[] = u"file://";

}  // namespace

bool IsWebUrl(std::u16string_view str) {
  return str.starts_with(u"http://") || str.starts_with(u"https://");
}

std::u16string MakeFileUrl(const std::filesystem::path& path) {
  return kFilePrefix + path.u16string();
}
