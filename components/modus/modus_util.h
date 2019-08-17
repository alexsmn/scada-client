#pragma once

#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "services/profile.h"
#include "window_definition.h"

inline bool IsModus2(const WindowDefinition& definition, Profile& profile) {
  bool modus2 = profile.modus2;
  if (auto* options = definition.FindItem("Options")) {
    auto version = options->GetInt("version", 0);
    if (version != 0)
      modus2 = version >= 2;
  }

  if (!base::LowerCaseEqualsASCII(definition.path.Extension(), ".xsde"))
    modus2 = false;

  return modus2;
}

inline bool IsModusFilePath(const base::FilePath& path) {
  const auto& ext = path.Extension();
  return base::EqualsCaseInsensitiveASCII(ext, L".sde") ||
         base::EqualsCaseInsensitiveASCII(ext, L".xsde");
}

inline std::optional<base::FilePath> MakeModusFilePath(
    const base::FilePath& hyperlink_path,
    const base::FilePath& current_display_path) {
  base::FilePath new_display_path;
  if (hyperlink_path.IsAbsolute()) {
    // This may be located outside of public directory.
    new_display_path = hyperlink_path;
  } else {
    // |current_display_path| is a relative public path.
    const auto& current_display_dir = current_display_path.DirName();
    // This is relative, but may be located outside of public directory.
    new_display_path = current_display_dir.Append(hyperlink_path);
  }

  base::FilePath result_path;
  if (!base::NormalizeFilePath(new_display_path, &result_path))
    return std::nullopt;

  return FullFilePathToPublic(result_path);
}