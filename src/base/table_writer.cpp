#include "base/table_writer.h"

#include "base/strings/string_util.h"

namespace {

// chrome/src/components/password_manager/core/browser/import/csv_writer.cc
std::string StringToCsv(const std::string& raw_value) {
  std::string result;
  result.reserve(raw_value.size());
  // Fields containing line breaks (CRLF), double quotes, and commas should be
  // enclosed in double-quotes. If double-quotes are used to enclose fields,
  // then double-quotes appearing inside a field must be escaped by preceding
  // them with another double quote.
  if (raw_value.find_first_of("\r\n\",") != std::string::npos) {
    result.push_back('\"');
    result.append(raw_value);
    base::ReplaceSubstringsAfterOffset(
        &result, result.size() - raw_value.size(), "\"", "\"\"");
    result.push_back('\"');
  } else {
    result.append(raw_value);
  }
  return result;
}

} // namespace

TableWriter::TableWriter()
    : skip_start_(true),
      start_of_line_(true) {
}

bool TableWriter::Init(const base::FilePath& path) {
  stream_.open(path.value());
  return !!stream_;
}

void TableWriter::StartRow() {
  auto skip = skip_start_;
  skip_start_ = false;
  if (skip)
    return;
  stream_ << std::endl;
  start_of_line_ = true;
}

void TableWriter::WriteCell(const base::StringPiece& str) {
  if (!start_of_line_)
    stream_ << ",";
  start_of_line_ = false;

  stream_ << StringToCsv(str.as_string());
}
