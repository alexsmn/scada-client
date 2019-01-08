#include "base/table_writer.h"

#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"

namespace {

// chrome/src/components/password_manager/core/browser/import/csv_writer.cc
std::string StringToCsv(base::StringPiece raw_value) {
  std::string result;
  result.reserve(raw_value.size());
  // Fields containing line breaks (CRLF), double quotes, and commas should be
  // enclosed in double-quotes. If double-quotes are used to enclose fields,
  // then double-quotes appearing inside a field must be escaped by preceding
  // them with another double quote.
  if (raw_value.find_first_of("\r\n\",") != std::string::npos) {
    result.push_back('\"');
    result.append(raw_value.begin(), raw_value.end());
    base::ReplaceSubstringsAfterOffset(
        &result, result.size() - raw_value.size(), "\"", "\"\"");
    result.push_back('\"');
  } else {
    result.append(raw_value.begin(), raw_value.end());
  }
  return result;
}

}  // namespace

TableWriter::TableWriter(std::ostream& stream) : stream_{stream} {}

void TableWriter::StartRow() {
  auto skip = skip_start_;
  skip_start_ = false;
  if (skip)
    return;

  stream_ << std::endl;
  if (!stream_)
    throw std::runtime_error("Write error");

  start_of_line_ = true;
}

void TableWriter::WriteCell(base::StringPiece16 str) {
  if (!start_of_line_)
    stream_ << L",";
  start_of_line_ = false;

  stream_ << StringToCsv(base::SysWideToNativeMB(str.as_string()));
  if (!stream_)
    throw std::runtime_error("Write error");
}
