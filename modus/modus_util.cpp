#include "modus/modus_util.h"

#include "base/strings/string_util.h"
#include "profile/window_definition.h"
#include "filesystem/file_util.h"
#include "profile/profile.h"

bool IsModus2(const WindowDefinition& definition, Profile& profile) {
  bool modus2 = profile.modus.modus2;
  if (auto* options = definition.FindItem("Options")) {
    auto version = options->GetInt("version", 0);
    if (version != 0)
      modus2 = version >= 2;
  }

  if (!base::LowerCaseEqualsASCII(definition.path.extension().string(),
                                  ".xsde")) {
    modus2 = false;
  }

  return modus2;
}

bool IsModusFilePath(const std::filesystem::path& path) {
  auto ext = path.extension().string();
  return base::EqualsCaseInsensitiveASCII(ext, ".sde") ||
         base::EqualsCaseInsensitiveASCII(ext, ".xsde");
}

std::optional<std::filesystem::path> MakeModusFilePath(
    const std::filesystem::path& hyperlink_path,
    const std::filesystem::path& current_display_path) {
  std::filesystem::path new_display_path;
  if (hyperlink_path.is_absolute()) {
    // This may be located outside of public directory.
    new_display_path = hyperlink_path;
  } else {
    // |current_display_path| is a relative public path.
    const auto& current_display_dir = current_display_path.parent_path();
    // This is relative, but may be located outside of public directory.
    new_display_path = current_display_dir / hyperlink_path;
  }

  new_display_path = new_display_path.lexically_normal();

  return FullFilePathToPublic(new_display_path);
}
