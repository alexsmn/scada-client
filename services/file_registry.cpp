#include "services/file_registry.h"

#include "base/string_piece_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

#include <cassert>

void FileRegistry::RegisterType(int id,
                                std::string name,
                                std::string_view extensions) {
  assert(type_map_.find(id) == type_map_.end());

  TypeEntry& entry = type_map_[id];
  entry.type_id = id;
  entry.name = std::move(name);
  entry.extensions =
      base::SplitString(AsStringPiece(extensions), ";", base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_NONEMPTY);
}

const FileRegistry::TypeEntry* FileRegistry::FindTypeById(int id) const {
  auto i = type_map_.find(id);
  return i != type_map_.end() ? &i->second : nullptr;
}

const FileRegistry::TypeEntry* FileRegistry::FindTypeByName(
    std::string_view name) const {
  for (auto& [id, entry] : type_map_) {
    if (base::EqualsCaseInsensitiveASCII(entry.name, AsStringPiece(name)))
      return &entry;
  }
  return nullptr;
}

const FileRegistry::TypeEntry* FileRegistry::FindTypeByExtension(
    std::string_view ext) const {
  for (auto& [id, entry] : type_map_) {
    for (auto& e : entry.extensions) {
      if (base::EqualsCaseInsensitiveASCII(e, AsStringPiece(ext)))
        return &entry;
    }
  }
  return nullptr;
}
