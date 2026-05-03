#pragma once

#include "aui/models/tree_node_model.h"
#include "favorites/favourites.h"

class FavouritesFolderNode;
class FavouritesWindowNode;

class FavouritesNode : public aui::TreeNode<FavouritesNode> {
 public:
  explicit FavouritesNode(Favourites& favourites) : favourites_{favourites} {}

  virtual void Delete() {}

  virtual FavouritesWindowNode* AsWindowNode() { return nullptr; }
  virtual const FavouritesWindowNode* AsWindowNode() const { return nullptr; }

  virtual FavouritesFolderNode* AsFolderNode() { return nullptr; }
  virtual const FavouritesFolderNode* AsFolderNode() const { return nullptr; }

 protected:
  Favourites& favourites_;
};

class FavouritesRootNode : public FavouritesNode {
 public:
  explicit FavouritesRootNode(Favourites& favourites);

  int FindFolderNode(const Page& folder) const;

  // TreeNode
  virtual std::u16string GetText(int column_id) const override {
    return std::u16string();
  }
};

class FavouritesFolderNode : public FavouritesNode {
 public:
  FavouritesFolderNode(Favourites& favourites, const Page& folder);

  const Page& folder() const { return folder_; }

  int FindWindowNode(const WindowDefinition& window) const;

  // FavouritesNode
  virtual std::u16string GetText(int column_id) const override {
    return folder_.GetTitle();
  }
  virtual int GetIcon() const override { return 2; }
  virtual void SetText(int column_id, const std::u16string& title) override;
  virtual bool IsEditable(int column_id) const override { return true; }
  virtual void Delete() override;
  virtual FavouritesFolderNode* AsFolderNode() override { return this; }
  virtual const FavouritesFolderNode* AsFolderNode() const override {
    return this;
  }

  const Page& folder_;
};

class FavouritesWindowNode : public FavouritesNode {
 public:
  FavouritesWindowNode(Favourites& favourites,
                       const Page& folder,
                       const WindowInfo& window_info,
                       const WindowDefinition& window_def)
      : FavouritesNode{favourites},
        folder_{folder},
        window_info_{window_info},
        window_def_{window_def} {}

  const WindowInfo& window_info() const { return window_info_; }
  const WindowDefinition& window_def() const { return window_def_; }

  // FavouritesNode
  virtual std::u16string GetText(int column_id) const override {
    return window_def_.GetTitle(window_info_);
  }
  virtual int GetIcon() const override;
  virtual void SetText(int column_id, const std::u16string& title) override;
  virtual bool IsEditable(int column_id) const override { return true; }
  virtual void Delete() override;
  virtual FavouritesWindowNode* AsWindowNode() override { return this; }
  virtual const FavouritesWindowNode* AsWindowNode() const override {
    return this;
  }

 private:
  const Page& folder_;
  const WindowInfo& window_info_;
  const WindowDefinition& window_def_;
};

class FavouritesTreeModel : public aui::TreeNodeModel<FavouritesNode>,
                            protected Favourites::Observer {
 public:
  explicit FavouritesTreeModel(Favourites& favourites);
  ~FavouritesTreeModel();

  FavouritesRootNode& root() {
    return *reinterpret_cast<FavouritesRootNode*>(
        aui::TreeNodeModel<FavouritesNode>::root());
  }

 protected:
  // Favorites::Observer
  virtual void OnFolderAdded(const Page& folder) override;
  virtual void OnFolderDeleted(const Page& folder) override;
  virtual void OnFolderChanged(const Page& folder) override;
  virtual void OnFavouriteAdded(const Page& folder,
                                const WindowDefinition& window) override;
  virtual void OnFavouriteDeleted(const Page& folder,
                                  const WindowDefinition& window) override;
  virtual void OnWindowChanged(const Page& folder,
                               const WindowDefinition& window) override;

 private:
  Favourites& favourites_;
};
