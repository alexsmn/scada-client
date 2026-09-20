#pragma once

#include <filesystem>
#include <optional>

// `IsModus2` and `DocumentKindFor` lived here until backlog 491. They turned
// the «Use Modus runtime renderer» profile flag and a window definition's
// `Options/version` into a `DocumentKind`, which the reader then ignored: SDE
// and XSDE are unrelated encodings, so only the file extension can choose
// between them, and it always did. The flag therefore decided nothing, and
// the one case where it carried information -- an `.xsde` display with the
// flag off -- was the one it got wrong.

// True for the two Modus document extensions, case-insensitively.
bool IsModusFilePath(const std::filesystem::path& path);

// Resolves a hyperlink taken from a Modus display into a public-relative path.
// `current_display_path` is the public-relative path of the display holding the
// hyperlink, so a relative `hyperlink_path` resolves against its directory.
// Returns nullopt when the target lies outside the public directory, which is
// not addressable and must not be opened.
std::optional<std::filesystem::path> MakeModusFilePath(
    const std::filesystem::path& hyperlink_path,
    const std::filesystem::path& current_display_path);
