#pragma once

#include "base/strings/string_piece.h"

#include <istream>

class TableReader {
 public:
  // |signature| is expected contents of the first cell useful to determine separrator.
  TableReader(std::wistream& stream, base::StringPiece16 signature = {});

  int row_index() const { return row_index_; }
  int cell_index() const { return cell_index_; }

  bool NextRow();
  bool NextCell(base::string16& str);

 private:
  base::StringPiece16 signature_;
  std::wistream& stream_;
  wchar_t separator_ = L',';
  base::string16 line_;
  base::string16::size_type line_pos_ = 0;
  bool has_cells_ = false;
  int row_index_ = 0;
  int cell_index_ = 1;
};
