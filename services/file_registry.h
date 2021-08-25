#pragma once

#include "base/string_piece_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

#include <unordered_map>

class FileRegistry {
 public:
  void RegisterType(int id, std::string name, std::string_view extensions);

  struct TypeEntry {
    int type_id;
    std::string name;
    std::vector<std::string> extensions;
  };

  const TypeEntry* FindTypeById(int id) const;
  const TypeEntry* FindTypeByName(std::string_view name) const;
  const TypeEntry* FindTypeByExtension(std::string_view ext) const;

 private:
  std::unordered_map<int, TypeEntry> type_map_;
};

inline void FileRegistry::RegisterType(int id,
                                       std::string name,
                                       std::string_view extensions) {
  assert(type_map_.find(id) == type_map_.end());

  TypeEntry& entry = type_map_[id];
  entry.type_id = id;
  entry.name = std::move(name);
  entry.extensions =
      base::SplitString(ToStringPiece(extensions), ";", base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_NONEMPTY);
}

inline const FileRegistry::TypeEntry* FileRegistry::FindTypeById(int id) const {
  auto i = type_map_.find(id);
  return i != type_map_.end() ? &i->second : nullptr;
}

inline const FileRegistry::TypeEntry* FileRegistry::FindTypeByName(
    std::string_view name) const {
  for (auto& [id, entry] : type_map_) {
    if (base::EqualsCaseInsensitiveASCII(entry.name, ToStringPiece(name)))
      return &entry;
  }
  return nullptr;
}

inline const FileRegistry::TypeEntry* FileRegistry::FindTypeByExtension(
    std::string_view ext) const {
  for (auto& [id, entry] : type_map_) {
    for (auto& e : entry.extensions) {
      if (base::EqualsCaseInsensitiveASCII(e, ToStringPiece(ext)))
        return &entry;
    }
  }
  return nullptr;
}
