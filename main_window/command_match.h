#pragma once

#include "aui/text_fold.h"

#include <span>
#include <string>
#include <vector>

// One selectable command in the palette. `title` is what the user reads and
// searches; `detail` (e.g. a keyboard shortcut) is shown right-aligned and is
// ignored by matching. Kept Qt-free so the matching logic can be unit-tested
// without a running UI.
struct CommandEntry {
  unsigned command_id = 0;
  std::u16string title;
  std::u16string detail;
};

// `FoldForSearch` moved to `aui/text_fold.h` when the Settings surface's search
// box needed the same folding; it is re-exported through this include so the
// palette's callers and its tests did not have to move with it.

// Returns the entries whose folded title contains the folded `query`, best
// matches first: titles that start with the query rank above interior matches,
// then by match position, then shorter titles, then title order. An empty
// query keeps every entry, ordered alphabetically by title.
std::vector<CommandEntry> RankCommandMatches(
    std::span<const CommandEntry> entries,
    std::u16string_view query);
