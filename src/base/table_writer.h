#pragma once

#include <fstream>

#include "base/files/file_path.h"

class TableWriter {
 public:
  TableWriter();

  bool Init(const base::FilePath& path);

  void StartRow();
  void WriteCell(const base::StringPiece& str);

 private:
  std::ofstream stream_;
  bool skip_start_;
  bool start_of_line_;
};
