#pragma once

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
