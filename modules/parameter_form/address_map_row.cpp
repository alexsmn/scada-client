#include "parameter_form/address_map_row.h"

#include <map>
#include <utility>

std::vector<bool> FindDuplicateAddresses(std::span<const AddressMapRow> rows) {
  std::map<std::pair<AddressMapRow::Kind, std::u16string>, int> counts;
  for (const AddressMapRow& row : rows) {
    if (!row.ioa.empty())
      ++counts[{row.kind, row.ioa}];
  }
  std::vector<bool> duplicates;
  duplicates.reserve(rows.size());
  for (const AddressMapRow& row : rows) {
    duplicates.push_back(!row.ioa.empty() && counts[{row.kind, row.ioa}] > 1);
  }
  return duplicates;
}
