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

  void RegisterType(int id, const base::string16& name, const base::string16& extensions);

  void Init();

  struct FileEntry {
    base::FilePath path;
    base::string16 title;
    ItemMap items;
  };

  class FileList : public std::vector<FileEntry> {
   public:
    int Find(const base::FilePath& path) const;

    std::vector<DisplayItem> GetFilesContainingItem(const scada::NodeId& item_id) const;
  };

  const FileList& GetList(int type_id) const;

  void Remove(const base::FilePath& path);

  void Refresh();

  void Update(int type_id, const base::FilePath& path, const base::string16& title,
              ItemMap& items);
  
 private:
  struct TypeEntry {
    base::string16 name;
    std::vector<base::string16> extensions;
    FileList list;
  };

  // Map of type id on TypeEntry.
  typedef std::map<int, TypeEntry> TypeMap;

  TypeMap::iterator FindTypeByName(const base::string16& name);
  TypeMap::iterator FindTypeByExtension(const base::string16& ext);

  FileList& GetMutableList(int type_id);

  void Load();
  void Save();

  TypeMap type_map_;

  DISALLOW_COPY_AND_ASSIGN(FileCache);
};

base::FilePath GetPublicFilePath(const base::FilePath& path);
base::FilePath FullFilePathToPublic(const base::FilePath& path);
