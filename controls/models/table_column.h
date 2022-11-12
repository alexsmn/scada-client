#pragma once

#include "controls/color.h"

namespace aui {

struct TableColumn {
  enum Alignment { LEFT, RIGHT, CENTER };
  enum class DataType { General, DateTime };

  int id = 0;
  std::u16string title;
  int width = 100;
  Alignment alignment = LEFT;
  DataType data_type = DataType::General;
};

struct GridCell {
  int row = 0;
  int column = 0;
  std::u16string text;
  Color text_color = ColorCode::Black;
  Color cell_color = ColorCode::White;
};

}  // namespace aui
