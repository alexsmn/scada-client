#include "filesystem/file_cache.h"

#include "base/boost_log.h"
#include "base/client_paths.h"
#include "base/format.h"
#include "base/json.h"
#include "base/logger.h"
#include "base/path_service.h"
#include "base/value_util.h"
#include "filesystem/file_registry.h"
#include "filesystem/file_util.h"
#include "model/node_id_util.h"

#include <filesystem>
#include <optional>

namespace {

std::filesystem::path GetCachePath() {
  std::filesystem::path path;
  base::PathService::Get(client::DIR_PUBLIC, &path);
  return path / "file-cache.json";
}

FileCache::FileEntry LoadFileEntry(const boost::json::value& data) {
  std::filesystem::path path{GetString16(data, "path")};

  FileCache::FileEntry entry{path, GetString16(data, "title")};

  // Old format could contain empty title.
  if (entry.title.empty())
    entry.title = path.stem().u16string();

  if (auto* items_data = FindDict(data, "items")) {
    for (const auto& [key, value] : items_data->as_object()) {
      auto node_id = NodeIdFromScadaString(key);
      int tag = value.is_int64() ? static_cast<int>(value.as_int64()) : -1;
      entry.items.try_emplace(std::move(node_id), tag);
    }
  }

  return entry;
}

boost::json::value SaveFileEntry(const FileCache::FileEntry& entry,
                                  std::string_view type_name) {
  boost::json::value file_data{boost::json::object{}};

  SetKey(file_data, "type", type_name);
  SetKey(file_data, "path", entry.path.u16string());
  if (!entry.title.empty())
    SetKey(file_data, "title", entry.title);

  if (!entry.items.empty()) {
    boost::json::value items_data{boost::json::object{}};
    for (auto& [node_id, key] : entry.items)
      SetKey(items_data, NodeIdToScadaString(node_id), key);
    file_data.as_object()["items"] = std::move(items_data);
  }

  return file_data;
}

}  // namespace

FileCache::FileCache(const FileRegistry& file_registry)
    : file_registry_{file_registry} {}

FileCache::~FileCache() {
  Save();
}

void FileCache::Init() {
  Load();
  Refresh();
}

int FileCache::FileList::Find(const std::filesystem::path& path) const {
  for (size_t i = 0; i < size(); ++i) {
    if (at(i).path == path)
      return static_cast<int>(i);
  }
  return -1;
}

void FileCache::Load() {
  std::filesystem::path cache_path = GetCachePath();
  BOOST_LOG_TRIVIAL(info) << "Loading file cache from " << cache_path.string();

  std::string error_message;
  auto data = LoadJsonFromFile(cache_path, &error_message);
  if (!data) {
    BOOST_LOG_TRIVIAL(error) << "Error on load file cache - " << error_message;
    return;
  }

  if (data->is_array()) {
    for (auto& file_data : data->as_array()) {
      if (auto* type =
              file_registry_.FindTypeByName(GetString(file_data, "type"))) {
        auto entry = LoadFileEntry(file_data);
        // Skip removed displays.
        if (!std::filesystem::exists(GetPublicFilePath(entry.path)))
          continue;
        auto& file_list = file_map_[type->type_id];
        file_list.emplace_back(std::move(entry));
      }
    }
  }
}

void FileCache::Save() {
  auto cache_path = GetCachePath();
  BOOST_LOG_TRIVIAL(info) << "Saving file cache to " << cache_path.string();

  boost::json::array list_data;
  for (auto& [type_id, file_list] : file_map_) {
    auto* type = file_registry_.FindTypeById(type_id);
    if (!type)
      continue;

    for (auto& entry : file_list)
      list_data.emplace_back(SaveFileEntry(entry, type->name));
  }

  SaveJsonToFile(boost::json::value{std::move(list_data)}, cache_path);
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
  std::filesystem::path public_path;
  base::PathService::Get(client::DIR_PUBLIC, &public_path);

  // Add new files.
  for (const auto& entry : std::filesystem::directory_iterator(public_path)) {
    if (!entry.is_regular_file())
      continue;
    auto full_path = entry.path();
    auto path = full_path.filename();
    auto* type = file_registry_.FindTypeByExtension(path.extension().string());
    if (!type)
      continue;

    auto& file_list = file_map_[type->type_id];
    if (file_list.Find(path) != -1)
      continue;

    std::u16string title = path.stem().u16string();
    file_list.emplace_back(FileEntry{std::move(path), std::move(title)});
  }
}

void FileCache::Update(int type_id,
                       const std::filesystem::path& path,
                       const std::u16string& title,
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

void FileCache::Remove(const std::filesystem::path& path) {
  auto* type = file_registry_.FindTypeByExtension(path.extension().string());
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
