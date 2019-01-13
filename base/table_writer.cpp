#include "base/table_writer.h"

#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"

namespace {

// chrome/src/components/password_manager/core/browser/import/csv_writer.cc
base::string16 StringToCsv(base::StringPiece16 raw_value,
                           base::char16 delimiter,
                           base::char16 quote) {
  base::string16 result;
  result.reserve(raw_value.size());
  // Fields containing line breaks (CRLF), double quotes, and commas should be
  // enclosed in double-quotes. If double-quotes are used to enclose fields,
  // then double-quotes appearing inside a field must be escaped by preceding
  // them with another double quote.
  const base::char16 escape[] = {L'\r', L'\n', delimiter, quote, L'\0'};
  if (raw_value.find_first_of(escape) != base::string16::npos) {
    result.push_back(quote);
    result.append(raw_value.begin(), raw_value.end());
    const base::char16 two_quotes[] = {quote, quote, L'\0'};
    base::ReplaceSubstringsAfterOffset(
        &result, result.size() - raw_value.size(),
        base::StringPiece16{&quote, 1}, two_quotes);
    result.push_back(quote);
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

void TableWriter::WriteCell(base::StringPiece16 utf16) {
  if (!start_of_line_)
    stream_ << delimiter;
  start_of_line_ = false;

  auto escaped = StringToCsv(utf16, delimiter, quote);

  std::string encoded =
      unicode ? base::UTF16ToUTF8(escaped) : base::SysWideToNativeMB(escaped);

  stream_ << encoded;
  if (!stream_)
    throw std::runtime_error("Write error");
}
