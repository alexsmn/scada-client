#pragma once

#include "aui/color.h"

#include <optional>

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
  // Rendered in the design-system monospace font — the plain look keeps
  // the default font.
  bool monospace = false;
};

struct GridCell {
  int row = 0;
  int column = 0;
  std::u16string text;
  // Transparent means "unstyled": the grid adapter falls through to the theme
  // palette. Models set explicit colours only for semantic cells (read-only
  // grey, blink yellow, user formats).
  Color text_color = ColorCode::Transparent;
  Color cell_color = ColorCode::Transparent;
  // Horizontal alignment for this cell alone. Unset — the usual case — means
  // the cell follows its column (`HeaderModel::GetAlignment`). Only a model
  // whose cells carry their own formatting sets it; the spreadsheet does,
  // because alignment there is a per-cell property the operator chooses.
  std::optional<TableColumn::Alignment> alignment;
};

}  // namespace scada::aui
