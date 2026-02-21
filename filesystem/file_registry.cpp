#include "filesystem/file_registry.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <cassert>

void FileRegistry::RegisterType(int id,
                                std::string_view name,
                                std::string_view extensions) {
  assert(type_map_.find(id) == type_map_.end());

  TypeEntry& entry = type_map_[id];
  entry.type_id = id;
  entry.name = name;
  boost::split(entry.extensions, extensions, boost::is_any_of(";"));
  for (auto& e : entry.extensions)
    boost::trim(e);
  std::erase_if(entry.extensions, [](const auto& s) { return s.empty(); });
}

const FileRegistry::TypeEntry* FileRegistry::FindTypeById(int id) const {
  auto i = type_map_.find(id);
  return i != type_map_.end() ? &i->second : nullptr;
}

const FileRegistry::TypeEntry* FileRegistry::FindTypeByName(
    std::string_view name) const {
  for (auto& [id, entry] : type_map_) {
    if (boost::iequals(entry.name, name))
      return &entry;
  }
  return nullptr;
}

const FileRegistry::TypeEntry* FileRegistry::FindTypeByExtension(
    std::string_view ext) const {
  for (auto& [id, entry] : type_map_) {
    for (auto& e : entry.extensions) {
      if (boost::iequals(e, ext))
        return &entry;
    }
  }
  return nullptr;
}
