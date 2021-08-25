#include "services/file_cache.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/format.h"
#include "base/logger.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "client_paths.h"
#include "client_utils.h"
#include "model/node_id_util.h"
#include "services/file_registry.h"
#include "value_util.h"

#include <optional>

namespace {

base::FilePath GetCachePath() {
  base::FilePath path;
  base::PathService::Get(client::DIR_PUBLIC, &path);
  return path.Append(FILE_PATH_LITERAL("file-cache.json"));
}

FileCache::FileEntry LoadFileEntry(const base::Value& data) {
  base::FilePath path{GetString16(data, "path")};

  FileCache::FileEntry entry{path, GetString16(data, "title")};

  // Old format could contain empty title.
  if (entry.title.empty())
    entry.title = path.RemoveExtension().value();

  if (auto* items_data = GetDict(data, "items")) {
    for (const auto& [key, value] : items_data->DictItems()) {
      auto node_id = NodeIdFromScadaString(key);
      int tag = value.is_int() ? value.GetInt() : -1;
      entry.items.try_emplace(std::move(node_id), tag);
    }
  }

  return entry;
}

base::Value SaveFileEntry(const FileCache::FileEntry& entry,
                          std::string_view type_name) {
  base::Value file_data{base::Value::Type::DICTIONARY};

  SetKey(file_data, "type", type_name);
  SetKey(file_data, "path", entry.path.value());
  if (!entry.title.empty())
    SetKey(file_data, "title", entry.title);

  if (!entry.items.empty()) {
    base::Value items_data{base::Value::Type::DICTIONARY};
    for (auto& [node_id, key] : entry.items)
      SetKey(items_data, NodeIdToScadaString(node_id), key);
    file_data.SetKey("items", std::move(items_data));
  }

  return file_data;
}

}  // namespace

FileCache::FileCache(const FileRegistry& file_registry)
    : file_registry_{file_registry} {
  Load();
  Refresh();
}

FileCache::~FileCache() {
  Save();
}

int FileCache::FileList::Find(const base::FilePath& path) const {
  for (size_t i = 0; i < size(); ++i) {
    if (at(i).path == path)
      return static_cast<int>(i);
  }
  return -1;
}

void FileCache::Load() {
  base::FilePath cache_path = GetCachePath();
  LOG(INFO) << "Loading file cache from " << cache_path.value();

  std::string error_message;
  auto data = LoadJsonFromFile(cache_path, &error_message);
  if (!data) {
    LOG(ERROR) << "Error on load file cache - " << error_message;
    return;
  }

  if (data->is_list()) {
    for (auto& file_data : data->GetList()) {
      if (auto* type =
              file_registry_.FindTypeByName(GetString(file_data, "type"))) {
        auto entry = LoadFileEntry(file_data);
        // Skip removed displays.
        if (!base::PathExists(GetPublicFilePath(entry.path)))
          continue;
        auto& file_list = file_map_[type->type_id];
        file_list.emplace_back(std::move(entry));
      }
    }
  }
}

void FileCache::Save() {
  base::FilePath cache_path = GetCachePath();
  LOG(INFO) << "Saving file cache to " << cache_path.value();

  base::Value::ListStorage list_data;
  for (auto& [type_id, file_list] : file_map_) {
    auto* type = file_registry_.FindTypeById(type_id);
    if (!type)
      continue;

    for (auto& entry : file_list)
      list_data.emplace_back(SaveFileEntry(entry, type->name));
  }

  SaveJsonToFile(base::Value{std::move(list_data)}, cache_path);
}

std::vector<FileCache::DisplayItem> FileCache::FileList::GetFilesContainingItem(
    const scada::NodeId& item_id) const {
  std::vector<DisplayItem> items;
  items.reserve(size());

  for (FileList::const_iterator i = begin(); i != end(); ++i) {
    const FileEntry& entry = *i;
    ItemMap::const_iterator j = entry.items.find(item_id);
    if (j != entry.items.end())
      items.push_back(DisplayItem(entry.path, j->second));
  }

  return items;
}

void FileCache::Refresh() {
  base::FilePath public_path;
  base::PathService::Get(client::DIR_PUBLIC, &public_path);

  // Add new files.
  // WARNING: FileEnumerator matches ".tsdt" by "*.tsd". Need to check
  // extension explicitly.
  base::FileEnumerator find(public_path, false, base::FileEnumerator::FILES);
  base::FilePath full_path;
  while (!(full_path = find.Next()).empty()) {
    auto path = full_path.BaseName();
    auto* type = file_registry_.FindTypeByExtension(
        base::SysWideToNativeMB(path.Extension()));
    if (!type)
      continue;

    auto& file_list = file_map_[type->type_id];
    if (file_list.Find(path) != -1)
      continue;

    std::wstring title = path.RemoveExtension().value();
    file_list.emplace_back(FileEntry{std::move(path), std::move(title)});
  }
}

void FileCache::Update(int type_id,
                       const base::FilePath& path,
                       const std::wstring& title,
                       ItemMap& items) {
  FileList& list = GetMutableList(type_id);

  FileEntry* entry = NULL;

  int index = list.Find(path);
  if (index == -1) {
    list.push_back(FileEntry());
    entry = &list.back();
    entry->path = path;
  } else {
    entry = &list[index];
  }

  entry->title = title;
  entry->items.swap(items);
  items.clear();
}

void FileCache::Remove(const base::FilePath& path) {
  auto* type = file_registry_.FindTypeByExtension(
      base::SysWideToNativeMB(path.Extension()));
  if (!type)
    return;

  auto& file_list = file_map_[type->type_id];
  int i = file_list.Find(path);
  if (i != -1)
    file_list.erase(file_list.begin() + i);
}

const FileCache::FileList& FileCache::GetList(int type_id) const {
  static FileList kEmptyList;
  auto i = file_map_.find(type_id);
  return i != file_map_.end() ? i->second : kEmptyList;
}

FileCache::FileList& FileCache::GetMutableList(int type_id) {
  return file_map_[type_id];
}
