#include "services/favourites.h"

#include "base/utils.h"

const Page* Favourites::GetFolder(const base::char16* folder) const {
  if (!folder)
    folder = L"";

  for (Folders::const_iterator i = folders_.begin(); i != folders_.end(); ++i)
    if (_wcsicmp(i->title.c_str(), folder) == 0)
      return &*i;
  return NULL;
}

const Page& Favourites::GetOrAddFolder(const base::char16* name) {
  const Page* folder = GetFolder(name);
  if (!folder) {
    Page new_folder;
    if (name)
      new_folder.title = name;
    folders_.push_back(new_folder);

    folder = &folders_.back();
    NotifyFolderAdded(*folder);
  }

  return *folder;
}

void Favourites::DeleteFolder(const Page& folder) {
  for (Folders::iterator i = folders_.begin(); i != folders_.end(); ++i) {
    if (&*i == &folder) {
      NotifyFolderDeleted(*i);
      folders_.erase(i);
      return;
    }
  }
  assert(false);
}

void Favourites::Add(const WindowDefinition& win, const Page& folder) {
  Page& local_folder = const_cast<Page&>(folder);

  WindowDefinition& w = local_folder.AddWindow(win);
  NotifyWindowAdded(local_folder, w);
}

void Favourites::Delete(const WindowDefinition& win, const Page& folder) {
  Page& local_folder = const_cast<Page&>(folder);

  int index = local_folder.FindWindowDef(win);
  if (index == -1)
    return;

  for (auto& o : observers_)
    o.OnFavouriteDeleted(local_folder, win);

  local_folder.DeleteWindow(index);
}

void Favourites::NotifyWindowAdded(const Page& folder,
                                   const WindowDefinition& win) const {
  for (auto& o : observers_)
    o.OnFavouriteAdded(folder, win);
}

void Favourites::NotifyFolderAdded(const Page& folder) const {
  for (auto& o : observers_)
    o.OnFolderAdded(folder);
}

void Favourites::NotifyFolderDeleted(const Page& folder) const {
  for (auto& o : observers_)
    o.OnFolderDeleted(folder);
}

void Favourites::NotifyFolderChanged(const Page& folder) const {
  for (auto& o : observers_)
    o.OnFolderChanged(folder);
}

void Favourites::NotifyWindowChanged(const Page& folder,
                                     const WindowDefinition& window) const {
  for (auto& o : observers_)
    o.OnWindowChanged(folder, window);
}

void Favourites::Load(const base::Value& value) {
  if (!value.is_list())
    return;

  for (const auto& folder_data : value.GetList())
    folders_.emplace_back().Load(folder_data);
}

base::Value Favourites::Save() const {
  base::Value::ListStorage list;
  list.reserve(folders_.size());
  for (auto& folder : folders_)
    list.emplace_back(folder.Save(false));
  return base::Value{std::move(list)};
}
