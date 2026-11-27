#pragma once

#include <QString>

namespace scada::aui {

// The width a horizontal header section needs to show `title` in full: the
// header font's advance, the style's header margins, and room for the sort
// indicator. A column defaulted narrower than its own header drew the title
// centred and clipped at both ends («ройство (резе»), so the backup and control
// column pairs of a bulk grid could not be told apart (UX audit 2026-11-25,
// item 4.3). Callers take the larger of this and their configured width.
int HeaderTitleWidth(const QString& title);

}  // namespace scada::aui
