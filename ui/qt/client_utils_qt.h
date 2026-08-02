#pragma once

#include <QPixmap>

#include <unordered_set>

namespace scada::aui {
class MenuModel;
}

class QMenu;

// Populates |menu| from the AUI |model|. When |skip_command_ids| is non-null,
// command/check/radio items whose command id is in the set are omitted (used to
// suppress entries a caller-supplied menu already provides); the recursion
// carries the same set into submenus. Passing null preserves the legacy
// behavior for all other callers.
void BuildMenu(QMenu& menu,
               scada::aui::MenuModel& model,
               const std::unordered_set<int>* skip_command_ids = nullptr);

// The icon for a command/action id, rendered from its Lucide glyph and tinted
// to the application palette. `size` is in logical pixels; the pixmap carries
// the device pixel ratio, so ask for the size you will draw at rather than
// scaling the result. Returns a null pixmap for an unmapped id.
QPixmap LoadPixmap(unsigned resource_id, int size = 24);
