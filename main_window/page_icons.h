#pragma once

#include <span>
#include <string_view>

// The icon vocabulary an operator can pick from for a profile page.
//
// A page button in the activity rail is 52 px wide, which fits neither the
// page's title nor anything close to it. The rail used to draw the page's
// 1-based position instead — unambiguous, but it says nothing about what the
// page holds, so the operator learns the mapping by repetition rather than by
// looking. The icon carries the meaning and the ordinal moves to the tooltip,
// where it still names the Ctrl+N shortcut.
//
// Keys are stable strings written to the profile, deliberately decoupled from
// any enumerator so reordering this table cannot silently repoint saved pages.
// An unknown key — a profile written by a newer build — degrades to the
// ordinal, which is why there is no "unknown icon" glyph.
//
// Qt-free on purpose: `Page` persists the key and the Wt shell can read the
// same vocabulary. The key → asset mapping lives with the Qt rail.
struct PageIcon {
  // Persisted in the profile. Never reuse a key for a different meaning.
  std::string_view key;
  // English source string; call sites pass it through Translate().
  const char* label;
};

// The pickable icons, in menu order. Chosen to cover the surfaces a page
// usually holds rather than to exhaust the icon set — a long list turns
// picking into scanning.
std::span<const PageIcon> GetPageIcons();

// True when `key` names an icon this build can draw. Empty is not an icon: it
// means "no icon chosen", which is a valid, and the default, state.
bool IsPageIconKey(std::string_view key);
