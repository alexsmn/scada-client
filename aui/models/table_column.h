#pragma once

#include "aui/color.h"

namespace scada::aui {

struct TableColumn {
  enum Alignment { LEFT, RIGHT, CENTER };
  enum class DataType { General, DateTime };

  int id = 0;
  std::u16string title;
  int width = 100;
  Alignment alignment = LEFT;
  DataType data_type = DataType::General;
  // Render the column's cells in the design-system monospace value font
  // (`--font-mono`, docs/client/ux/design-language.md §3) so digits stay
  // tabular as values update. Timestamp columns (`DataType::DateTime`) render
  // monospace implicitly; set this for value / NodeId / measurement columns.
  // Only takes effect under the opt-in token themes — the legacy look keeps
  // the default font.
  bool monospace = false;
};

struct GridCell {
  int row = 0;
  int column = 0;
  std::u16string text;
  // Transparent means "unstyled": the grid adapter renders the palette's
  // colours (black-on-white in the legacy look, the theme tokens under the
  // reshell palette). Models set explicit colours only for semantic cells
  // (read-only grey, blink yellow, user formats).
  Color text_color = ColorCode::Transparent;
  Color cell_color = ColorCode::Transparent;
};

}  // namespace scada::aui
