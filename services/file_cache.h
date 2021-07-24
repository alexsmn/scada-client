#pragma once

#include "base/files/file_path.h"
#include "core/configuration_types.h"

#include <map>
#include <vector>

class FileCache {
 public:
  typedef int ItemTag;
  typedef std::map<scada::NodeId, ItemTag> ItemMap;

  typedef std::pair<base::FilePath, ItemTag> DisplayItem;

  FileCache();
  ~FileCache();

  FileCache(const FileCache&) = delete;
  FileCache& operator=(const FileCache&) = delete;

  void RegisterType(int id, std::string name, std::string extensions);

  void Init();

  struct FileEntry {
    base::FilePath path;
    std::wstring title;
    ItemMap items;
  };

  class FileList : public std::vector<FileEntry> {
   public:
    int Find(const base::FilePath& path) const;

    std::vector<DisplayItem> GetFilesContainingItem(
        const scada::NodeId& item_id) const;
  };

  const FileList& GetList(int type_id) const;

  void Remove(const base::FilePath& path);

  void Refresh();

  void Update(int type_id,
              const base::FilePath& path,
              const std::wstring& title,
              ItemMap& items);

 private:
  struct TypeEntry {
    std::string name;
    std::vector<std::string> extensions;
    FileList list;
  };

  // Map of type id on TypeEntry.
  typedef std::map<int, TypeEntry> TypeMap;

  TypeEntry* FindTypeByName(std::string_view name);
  TypeEntry* FindTypeByExtension(std::string_view ext);

  FileList& GetMutableList(int type_id);

  void Load();
  void Save();

  TypeMap type_map_;
};
