#include "favorites/favourites.h"

#include "base/check.h"
#include "base/utils.h"

const Page* Favourites::GetFolder(std::u16string_view name) const {
  for (auto& folder : folders_)
    if (folder.title == name)
      return &folder;
  return nullptr;
}

const Page& Favourites::GetOrAddFolder(std::u16string_view name) {
  const Page* folder = GetFolder(name);
  if (!folder) {
    Page new_folder;
    new_folder.title.assign(name.data(), name.size());
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
  base::NotReached();
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

  favourite_deleted_signal_(local_folder, win);

  local_folder.DeleteWindow(index);
}

void Favourites::NotifyWindowAdded(const Page& folder,
                                   const WindowDefinition& win) const {
  favourite_added_signal_(folder, win);
}

void Favourites::NotifyFolderAdded(const Page& folder) const {
  folder_added_signal_(folder);
}

void Favourites::NotifyFolderDeleted(const Page& folder) const {
  folder_deleted_signal_(folder);
}

void Favourites::NotifyFolderChanged(const Page& folder) const {
  folder_changed_signal_(folder);
}

void Favourites::NotifyWindowChanged(const Page& folder,
                                     const WindowDefinition& window) const {
  window_changed_signal_(folder, window);
}

void Favourites::Load(const boost::json::value& value) {
  if (!value.is_array())
    return;

  for (const auto& folder_data : value.as_array())
    folders_.emplace_back().Load(folder_data);
}

boost::json::value Favourites::Save() const {
  boost::json::array list;
  list.reserve(folders_.size());
  for (auto& folder : folders_)
    list.emplace_back(folder.Save(false));
  return boost::json::value{std::move(list)};
}
