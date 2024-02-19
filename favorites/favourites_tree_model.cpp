#include "favorites/favourites_tree_model.h"

#include "common_resources.h"
#include "controller/window_info.h"

class FavouritesFolderNode;
class FavouritesWindowNode;

FavouritesRootNode::FavouritesRootNode(Favourites& favourites)
    : FavouritesNode{favourites} {
  int n = 0;
  for (Favourites::Folders::const_iterator i = favourites_.folders().begin();
       i != favourites_.folders().end(); ++i, ++n) {
    Add(n, std::make_unique<FavouritesFolderNode>(favourites_, *i));
  }
}

int FavouritesRootNode::FindFolderNode(const Page& folder) const {
  for (int i = 0; i < GetChildCount(); ++i) {
    const FavouritesFolderNode& node =
        reinterpret_cast<const FavouritesFolderNode&>(GetChild(i));
    if (&node.folder_ == &folder)
      return i;
  }
  return -1;
}

FavouritesFolderNode::FavouritesFolderNode(Favourites& favourites,
                                           const Page& folder)
    : FavouritesNode{favourites}, folder_(folder) {
  for (int i = 0; i != folder.GetWindowCount(); ++i) {
    const WindowDefinition& window = folder.GetWindow(i);
    Add(i, std::make_unique<FavouritesWindowNode>(favourites_, folder, window));
  }
}

int FavouritesFolderNode::FindWindowNode(const WindowDefinition& window) const {
  for (int i = 0; i < GetChildCount(); ++i) {
    const FavouritesWindowNode* node = GetChild(i).AsWindowNode();
    if (node && &node->window() == &window)
      return i;
  }
  return -1;
}

void FavouritesFolderNode::SetText(int column_id, const std::u16string& title) {
  // TODO: Rename using intendent Favourites method.
  const_cast<Page&>(folder_).title = title;
  favourites_.NotifyFolderChanged(folder_);
}

void FavouritesFolderNode::Delete() {
  favourites_.DeleteFolder(folder_);
}

void FavouritesWindowNode::SetText(int column_id, const std::u16string& title) {
  const_cast<WindowDefinition&>(window_).title = title;
  favourites_.NotifyWindowChanged(folder_, window_);
}

int FavouritesWindowNode::GetIcon() const {
  switch (window_.window_info().command_id) {
    case ID_GRAPH_VIEW:
      return 1;
    case ID_TABLE_VIEW:
      return 0;
    default:
      return -1;
  }
}

void FavouritesWindowNode::Delete() {
  favourites_.Delete(window_, folder_);
}

// FavouritesTreeModel

FavouritesTreeModel::FavouritesTreeModel(Favourites& favourites)
    : TreeNodeModel<FavouritesNode>(
          std::make_unique<FavouritesRootNode>(favourites)),
      favourites_(favourites) {
  favourites_.AddObserver(*this);
}

FavouritesTreeModel::~FavouritesTreeModel() {
  favourites_.RemoveObserver(*this);
}

void FavouritesTreeModel::OnFavouriteAdded(const Page& folder,
                                           const WindowDefinition& window) {
  int i = root().FindFolderNode(folder);
  if (i == -1)
    return;

  FavouritesFolderNode& folder_node =
      reinterpret_cast<FavouritesFolderNode&>(root().GetChild(i));
  Add(folder_node, folder_node.GetChildCount(),
      std::make_unique<FavouritesWindowNode>(favourites_, folder, window));
}

void FavouritesTreeModel::OnFavouriteDeleted(const Page& folder,
                                             const WindowDefinition& window) {
  int i = root().FindFolderNode(folder);
  if (i == -1)
    return;

  FavouritesFolderNode& folder_node =
      reinterpret_cast<FavouritesFolderNode&>(root().GetChild(i));

  int index = folder_node.FindWindowNode(window);
  assert(index != -1);
  Remove(folder_node, index);
}

void FavouritesTreeModel::OnFolderChanged(const Page& folder) {
  int i = root().FindFolderNode(folder);
  if (i == -1)
    return;

  FavouritesFolderNode& folder_node =
      reinterpret_cast<FavouritesFolderNode&>(root().GetChild(i));
  TreeNodeChanged(&folder_node);
}

void FavouritesTreeModel::OnFolderAdded(const Page& folder) {
  Add(root(), root().GetChildCount(),
      std::make_unique<FavouritesFolderNode>(favourites_, folder));
}

void FavouritesTreeModel::OnFolderDeleted(const Page& folder) {
  int i = root().FindFolderNode(folder);
  if (i == -1)
    return;

  Remove(root(), i);
}

void FavouritesTreeModel::OnWindowChanged(const Page& folder,
                                          const WindowDefinition& window) {
  int i = root().FindFolderNode(folder);
  if (i == -1)
    return;

  FavouritesFolderNode& folder_node =
      reinterpret_cast<FavouritesFolderNode&>(root().GetChild(i));

  int index = folder_node.FindWindowNode(window);
  assert(index != -1);

  FavouritesWindowNode& window_node =
      reinterpret_cast<FavouritesWindowNode&>(folder_node.GetChild(index));
  TreeNodeChanged(&window_node);
}
