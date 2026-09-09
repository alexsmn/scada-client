#include "bulk_create/bulk_create_pattern.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>
#include <utility>

namespace {

// Widens an ASCII string (decimal/hex digits) to UTF-16.
std::u16string Widen(const std::string& ascii) {
  return std::u16string(ascii.begin(), ascii.end());
}

// Replaces every occurrence of `token` in `text` with `value`.
void ReplaceAll(std::u16string& text,
                std::u16string_view token,
                const std::u16string& value) {
  for (std::size_t pos = text.find(token); pos != std::u16string::npos;
       pos = text.find(token, pos + value.size())) {
    text.replace(pos, token.size(), value);
  }
}

}  // namespace

std::u16string ExpandTokens(std::u16string_view template_str, int index) {
  std::u16string result(template_str);
  // {nn} is replaced before {n} would matter; the tokens do not overlap as
  // substrings ("{n}" is not a substring of "{nn}"), so order is not critical.
  ReplaceAll(result, u"{nn}", Widen(std::format("{:02}", index)));
  ReplaceAll(result, u"{hex}", Widen(std::format("{:x}", index)));
  ReplaceAll(result, u"{n}", Widen(std::format("{}", index)));
  return result;
}

std::vector<BulkCreatePreviewRow> ExpandBulkCreate(
    const BulkCreateParams& params,
    const std::set<std::u16string>& existing_node_ids) {
  std::vector<BulkCreatePreviewRow> rows;
  if (params.count <= 0)
    return rows;

  rows.reserve(params.count);
  for (int i = 0; i < params.count; ++i) {
    const int index = params.start_index + i * params.index_step;

    BulkCreatePreviewRow row;
    row.number = index;
    row.name = ExpandTokens(params.name_template, index);
    row.node_id = ExpandTokens(params.node_id_template, index);
    // In 64-bit: the wizard's own spin boxes allow ioa_start up to 1e6,
    // count up to 1e5 and ioa_step up to 1e5, whose product overflows a
    // signed int -- undefined behaviour, reached by values the UI offers.
    const std::int64_t ioa = static_cast<std::int64_t>(params.ioa_start) +
                             static_cast<std::int64_t>(i) *
                                 static_cast<std::int64_t>(params.ioa_step);
    row.ioa_out_of_range = ioa > kMaxBulkCreateIoa || ioa < 0;
    row.ioa =
        static_cast<int>(std::clamp<std::int64_t>(ioa, 0, kMaxBulkCreateIoa));
    row.conflict = existing_node_ids.contains(row.node_id);
    rows.push_back(std::move(row));
  }
  return rows;
}

BulkCreateSummary SummarizeBulkCreate(
    const std::vector<BulkCreatePreviewRow>& rows) {
  BulkCreateSummary summary;
  for (const BulkCreatePreviewRow& row : rows) {
    if (row.ioa_out_of_range)
      ++summary.out_of_range_count;
    else if (row.conflict)
      ++summary.conflict_count;
    else
      ++summary.new_count;
  }
  return summary;
}
