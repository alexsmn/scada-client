#pragma once

#include <fstream>

#include "base/files/file_path.h"

class TableReader {
 public:
  TableReader();

  int row_index() const { return row_index_; }
  int cell_index() const { return cell_index_; }

  // |signature| is expected contents of the first cell useful to determine separrator.
  bool Init(const base::FilePath& path, base::StringPiece signature = {});

  bool NextRow();
  bool NextCell(std::string& str);

 private:
  base::StringPiece signature_;
  std::ifstream stream_;
  char separator_ = ';';
  std::string line_;
  std::string::size_type line_pos_;
  bool has_cells_;
  int row_index_;
  int cell_index_;
};
