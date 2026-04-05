#pragma once

#include <string>
#include <unordered_map>

// TODO: Rename to `FileTypeRegistry` or `FileExtensionRegistry`.
class FileRegistry {
 public:
  void RegisterType(int id, std::string_view name, std::string_view extensions);

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
