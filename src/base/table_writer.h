#pragma once

#include "base/strings/string_piece.h"

#include <ostream>

class TableWriter {
 public:
  explicit TableWriter(std::wostream& stream);

  void StartRow();
  void WriteCell(base::StringPiece16 str);

 private:
  std::wostream& stream_;
  bool skip_start_ = true;
  bool start_of_line_ = true;
};
