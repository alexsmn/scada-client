#include "services/favourites.h"

#include "base/utils.h"
#include "base/xml.h"

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

  int index = local_folder.FindWindow(win);
  if (index == -1)
    return;

  FOR_EACH_OBSERVER(Observer, observers_,
                    OnFavouriteDeleted(local_folder, win));

  local_folder.DeleteWindow(index);
}

void Favourites::NotifyWindowAdded(const Page& folder, const WindowDefinition& win) const {
  FOR_EACH_OBSERVER(Observer, observers_, OnFavouriteAdded(folder, win));
}

void Favourites::NotifyFolderAdded(const Page& folder) const {
  FOR_EACH_OBSERVER(Observer, observers_, OnFolderAdded(folder));
}

void Favourites::NotifyFolderDeleted(const Page& folder) const {
  FOR_EACH_OBSERVER(Observer, observers_, OnFolderDeleted(folder));
}

void Favourites::NotifyFolderChanged(const Page& folder) const {
  FOR_EACH_OBSERVER(Observer, observers_, OnFolderChanged(folder));
}

void Favourites::NotifyWindowChanged(const Page& folder,
                                     const WindowDefinition& window) const {
  FOR_EACH_OBSERVER(Observer, observers_, OnWindowChanged(folder, window));
}

void Favourites::Load(const xml::Node& root_node) {
  for (const xml::Node* node = root_node.first_child; node; node = node->next) {
    const xml::Node& pagee = *node;
    if (pagee.type != xml::NodeTypeElement)
      continue;
    if (pagee.name.compare("Folder") != 0)
      continue;

    Page folder;
    try {
      folder.Load(pagee);
    } catch (HRESULT err) {
      LOG(ERROR) << "Error " << static_cast<int>(err)
                 << " on load favorite folder " << folder.id;
      continue;
    }

    folders_.push_back(folder);
  }
}

void Favourites::Save(xml::Node& root_node) const {
  for (auto& folder : folders_) {
    xml::Node& pagee = root_node.AddElement("Folder");
    folder.Save(pagee, false);
  }
}
