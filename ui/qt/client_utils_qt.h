#pragma once

#include "aui/aui_ns_compat.h"

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
               aui::MenuModel& model,
               const std::unordered_set<int>* skip_command_ids = nullptr);

QPixmap LoadPixmap(unsigned resource_id);
