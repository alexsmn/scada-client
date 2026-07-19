#pragma once

#include <set>
#include <string>
#include <string_view>
#include <vector>

// Pure, node-service-free engine backing the reshell bulk-create wizard
// (client/docs/ui-mockups/screens/bulk-create.html). It expands a naming /
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

// One expanded preview row.
struct BulkCreatePreviewRow {
  int number = 0;             // the running index (start_index, +index_step ...)
  std::u16string name;        // expanded name_template
  std::u16string node_id;     // expanded node_id_template
  int ioa = 0;                // ioa_start + row * ioa_step
  bool conflict = false;      // node_id already exists in the address space
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
};

// Tallies the new vs. conflicting rows.
BulkCreateSummary SummarizeBulkCreate(
    const std::vector<BulkCreatePreviewRow>& rows);
