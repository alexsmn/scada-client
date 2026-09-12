#pragma once

#include "display/view/display_document.h"

#include <filesystem>
#include <optional>

class Profile;
class WindowDefinition;

// True when `definition` is to be rendered by the Modus version-2 renderer.
// The window definition's own `Options/version` wins where it is recorded;
// otherwise the operator's «Use Modus runtime renderer» profile flag decides.
// A document that is not `.xsde` is always version 1, whatever either says.
bool IsModus2(const WindowDefinition& definition, Profile& profile);

// The document kind to open `definition` with. This is how the version-2
// choice reaches the renderer: the controller passes the result instead of
// `DocumentKind::kAuto`, so the decision is made from the definition and the
// profile rather than re-guessed from the file extension.
//
// It returned `int32_t` until ADR 0012 phase 3, to keep the plugin's C API out
// of this header. The renderer is linked now and its kind is an enum class, so
// the indirection has nothing left to protect.
//
// Note what the renderer then does with it, which is backlog 491: the SDE and
// XSDE readers are unrelated encodings rather than two versions of one, so the
// loader still decides between them by extension and this value is carried but
// not obeyed.
scada::display::view::DocumentKind DocumentKindFor(
    const WindowDefinition& definition,
    Profile& profile);

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
