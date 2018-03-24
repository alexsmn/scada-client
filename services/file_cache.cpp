#include "services/file_cache.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/format.h"
#include "base/logger.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/sys_string_conversions.h"
#include "base/xml.h"
#include "client_paths.h"
#include "common/node_id_util.h"

#include <fstream>

base::FilePath GetPublicFilePath(const base::FilePath& path) {
  base::FilePath public_path;
  PathService::Get(client::DIR_PUBLIC, &public_path);
  return public_path.Append(path);
}

base::FilePath FullFilePathToPublic(const base::FilePath& path) {
  return path.BaseName();
}

namespace {

base::FilePath GetCachePath() {
  base::FilePath path;
  PathService::Get(client::DIR_PUBLIC, &path);
  return path.Append(FILE_PATH_LITERAL("file-cache.xml"));
}

void LoadFileList(FileCache::FileList& list, const xml::Node& parent) {
  for (const xml::Node* node = parent.first_child; node; node = node->next) {
    if (node->name != "Display" && node->name != "File")
      continue;

    const xml::Node& display_node = *node;
    base::FilePath path(display_node.GetAttribute("name"));

    // Skip removed displays.
    if (!base::PathExists(GetPublicFilePath(path)))
      continue;

    FileCache::FileEntry entry{path, display_node.GetAttribute("title")};
    // Old format could contain empty title.
    if (entry.title.empty())
      entry.title = path.RemoveExtension().value();

    for (const xml::Node* node = display_node.first_child; node;
         node = node->next) {
      if (node->name != "Item")
        continue;

      std::string path = node->GetAttributeA("path");

      int key = ParseWithDefault<int>(node->GetAttribute("key"), -1);
      if (key == -1)
        continue;

      auto item_id = NodeIdFromScadaString(path);
      if (item_id.is_null())
        continue;

      entry.items[item_id] = key;
    }

    list.push_back(std::move(entry));
  }
}

void SaveFileList(const FileCache::FileList& list, xml::Node& parent) {
  for (size_t i = 0; i < list.size(); ++i) {
    const FileCache::FileEntry& entry = list[i];

    xml::Node& display_node = parent.AddElement("File");

    display_node.SetAttribute("name", entry.path.value());
    if (!entry.title.empty())
      display_node.SetAttribute("title", entry.title);

    for (FileCache::ItemMap::const_iterator i = entry.items.begin();
         i != entry.items.end(); ++i) {
      const scada::NodeId& item = i->first;
      int key = i->second;

      xml::Node& item_node = display_node.AddElement("Item");
      item_node.SetAttribute("path", NodeIdToScadaString(item));
      item_node.SetAttribute("key", Format(key));
    }
  }
}

}  // namespace

FileCache::FileCache() {}

FileCache::~FileCache() {
  Save();
}

void FileCache::Init() {
  Load();
  Refresh();
}

void FileCache::RegisterType(int id,
                             const base::string16& name,
                             const base::string16& extensions) {
  DCHECK(type_map_.find(id) == type_map_.end());

  TypeEntry& entry = type_map_[id];
  entry.name = name;
  entry.extensions = base::SplitString(extensions, L";", base::TRIM_WHITESPACE,
                                       base::SPLIT_WANT_NONEMPTY);
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

  try {
    std::ifstream stream(cache_path.value().c_str(),
                         std::ios::in | std::ios::binary);
    xml::TextReader reader(stream);
    xml::Document cache_xml;
    cache_xml.Load(reader);

    const xml::Node* root = cache_xml.GetDocumentElement();
    if (!root)
      return;

    for (const xml::Node* e = root->first_child; e; e = e->next) {
      if (e->name != "Type")
        continue;

      base::string16 name = e->GetAttribute("name");
      TypeMap::iterator i = FindTypeByName(name);
      if (i != type_map_.end())
        LoadFileList(i->second.list, *e);
    }

  } catch (const std::exception& e) {
    LOG(ERROR) << "Error on load file cache - " << e.what();
  }
}

void FileCache::Save() {
  base::FilePath cache_path = GetCachePath();
  LOG(INFO) << "Saving file cache to " << cache_path.value();

  try {
    xml::Document cache_xml;
    cache_xml.SetEncoding(xml::EncodingUtf8);

    xml::Node& root = cache_xml.AddElement("Cache");

    for (TypeMap::const_iterator i = type_map_.begin(); i != type_map_.end();
         ++i) {
      const TypeEntry& type = i->second;
      if (type.list.empty())
        continue;

      xml::Node& e = root.AddElement("Type");
      e.SetAttribute("name", type.name);
      SaveFileList(type.list, e);
    }

    std::ofstream stream(cache_path.value().c_str(),
                         std::ios::out | std::ios::binary);
    xml::TextWriter writer(stream);
    cache_xml.Save(writer);

  } catch (const std::exception& e) {
    LOG(ERROR) << "Error on save file cache - " << e.what();
  }
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
  PathService::Get(client::DIR_PUBLIC, &public_path);

  // Add new files.
  // WARNING: FileEnumerator matches ".tsdt" by "*.tsd". Need to check
  // extension explicitly.
  base::FileEnumerator find(public_path, false, base::FileEnumerator::FILES);
  base::FilePath full_path;
  while (!(full_path = find.Next()).empty()) {
    base::FilePath path = full_path.BaseName();

    TypeMap::iterator i = FindTypeByExtension(path.Extension());
    if (i == type_map_.end())
      continue;

    FileList& list = i->second.list;
    if (list.Find(path) != -1)
      continue;

    base::string16 title = path.RemoveExtension().value();
    FileEntry entry = {path, title};
    list.push_back(entry);
  }
}

void FileCache::Update(int type_id,
                       const base::FilePath& path,
                       const base::string16& title,
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
  TypeMap::iterator p = FindTypeByExtension(path.Extension());
  if (p != type_map_.end())
    return;

  FileList& list = p->second.list;
  int i = list.Find(path);
  if (i != -1)
    list.erase(list.begin() + i);
}

const FileCache::FileList& FileCache::GetList(int type_id) const {
  return const_cast<FileCache*>(this)->GetMutableList(type_id);
}

FileCache::FileList& FileCache::GetMutableList(int type_id) {
  TypeMap::iterator i = type_map_.find(type_id);
  DCHECK(i != type_map_.end());
  return i->second.list;
}

FileCache::TypeMap::iterator FileCache::FindTypeByName(
    const base::string16& name) {
  for (TypeMap::iterator i = type_map_.begin(); i != type_map_.end(); ++i) {
    if (_wcsicmp(i->second.name.c_str(), name.c_str()) == 0)
      return i;
  }
  return type_map_.end();
}

FileCache::TypeMap::iterator FileCache::FindTypeByExtension(
    const base::string16& ext) {
  for (TypeMap::iterator i = type_map_.begin(); i != type_map_.end(); ++i) {
    TypeEntry& entry = i->second;
    for (size_t j = 0; j < entry.extensions.size(); ++j) {
      if (_wcsicmp(entry.extensions[j].c_str(), ext.c_str()) == 0)
        return i;
    }
  }
  return type_map_.end();
}
