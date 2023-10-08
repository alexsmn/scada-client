#pragma once

#include "scada/node_id.h"

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

class FileRegistry;

class FileCache {
 public:
  typedef int ItemTag;
  typedef std::map<scada::NodeId, ItemTag> ItemMap;

  using DisplayItem = std::pair<std::filesystem::path, ItemTag>;

  explicit FileCache(const FileRegistry& file_registry);
  ~FileCache();

  FileCache(const FileCache&) = delete;
  FileCache& operator=(const FileCache&) = delete;

  struct FileEntry {
    std::filesystem::path path;
    std::u16string title;
    ItemMap items;
  };

  class FileList : public std::vector<FileEntry> {
   public:
    int Find(const std::filesystem::path& path) const;

    std::vector<DisplayItem> GetFilesContainingItem(
        const scada::NodeId& item_id) const;
  };

  const FileList& GetList(int type_id) const;

  void Remove(const std::filesystem::path& path);

  void Refresh();

  void Update(int type_id,
              const std::filesystem::path& path,
              const std::u16string& title,
              ItemMap& items);

 private:
  FileList& GetMutableList(int type_id);

  void Load();
  void Save();

  const FileRegistry& file_registry_;

  std::unordered_map<int, FileList> file_map_;
};
