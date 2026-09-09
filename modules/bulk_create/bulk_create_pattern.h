#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

// Pure, node-service-free engine backing the reshell bulk-create wizard
// (docs/product/ui-mockups/screens/bulk-create.html). It expands a naming /
// addressing pattern with `{n}`-style tokens into the wizard's live-preview
// rows and flags the ones that collide with existing nodes. Unit-testable
// without Qt or a node service.

// Expands the index tokens in `template_str` for a 1-based `index`:
//   {n}    -> the index in decimal (1, 2, ... 24)
//   {nn}   -> the index zero-padded to at least two digits (01, 02, ... 24)
//   {hex}  -> the index in lowercase hexadecimal (1, 2, ... a, b ... 18)
// Any other text is copied verbatim, so "TS{n} current" with index 8 yields
// "TS8 current". Unknown `{...}` sequences are left untouched.
std::u16string ExpandTokens(std::u16string_view template_str, int index);

// The pattern the wizard's Naming/Addressing step edits.
struct BulkCreateParams {
  std::u16string name_template;     // e.g. "TS{n} current"
  std::u16string node_id_template;  // e.g. "ns=2;s=RTU.TS{n}.I"
  int start_index = 1;
  int count = 0;
  int index_step = 1;
  int ioa_start = 0;
  int ioa_step = 1;
};

// The largest address a row may carry. The destination is the rule's
// `TransmissionItemType_Address` property, a `scada::Int32`, and that is the
// only bound the client can apply: the property is generic over the Modbus,
// IEC 60870 and IEC 61850 transmission item subtypes, and for the 60870
// family the information object address width is a configured system
// parameter rather than a constant -- `IecProtocolOptions::len_addr`
// (third_party/iec60870/iec60870/model/types.h), default 3 octets but read
// and written at whatever length the link is configured for. So a 16 777 215
// bound here would be both too narrow for a wider link and too wide for a
// one-octet one, and the protocol-specific check belongs to the edge.
inline constexpr std::int64_t kMaxBulkCreateIoa = 0x7fffffff;

// One expanded preview row.
struct BulkCreatePreviewRow {
  int number = 0;          // the running index (start_index, +index_step ...)
  std::u16string name;     // expanded name_template
  std::u16string node_id;  // expanded node_id_template
  int ioa = 0;             // ioa_start + row * ioa_step, clamped
  bool conflict = false;   // node_id already exists in the address space
  // The expanded address ran past kMaxBulkCreateIoa. `ioa` is then clamped
  // and the row must not be created.
  bool ioa_out_of_range = false;
};

// Expands `params` into `params.count` preview rows, marking a row as a
// conflict when its expanded NodeId is already present in
// `existing_node_ids`. Returns an empty list when count <= 0.
std::vector<BulkCreatePreviewRow> ExpandBulkCreate(
    const BulkCreateParams& params,
    const std::set<std::u16string>& existing_node_ids);

// Aggregate counts for the wizard's "N new / M conflict" summary.
struct BulkCreateSummary {
  int new_count = 0;
  int conflict_count = 0;
  int out_of_range_count = 0;
};

// Tallies the new vs. conflicting rows.
BulkCreateSummary SummarizeBulkCreate(
    const std::vector<BulkCreatePreviewRow>& rows);
