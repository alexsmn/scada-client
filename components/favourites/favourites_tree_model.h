#pragma once

#include "services/favourites.h"
#include "ui/base/models/tree_node_model.h"

class FavouritesFolderNode;
class FavouritesWindowNode;

class FavouritesNode : public ui::TreeNode<FavouritesNode> {
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
  virtual base::string16 GetText(int column_id) const override {
    return base::string16();
  }
};

class FavouritesFolderNode : public FavouritesNode {
 public:
  FavouritesFolderNode(Favourites& favourites, const Page& folder);

  const Page& folder() const { return folder_; }

  int FindWindowNode(const WindowDefinition& window) const;

  // FavouritesNode
  virtual base::string16 GetText(int column_id) const override {
    return folder_.GetTitle();
  }
  virtual int GetIcon() const override { return 2; }
  virtual void SetText(int column_id, const base::string16& title) override;
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
                       const WindowDefinition& window)
      : FavouritesNode{favourites}, folder_{folder}, window_{window} {}

  const WindowDefinition& window() const { return window_; }

  // FavouritesNode
  virtual base::string16 GetText(int column_id) const override {
    return window_.GetTitle();
  }
  virtual int GetIcon() const override;
  virtual void SetText(int column_id, const base::string16& title) override;
  virtual bool IsEditable(int column_id) const override { return true; }
  virtual void Delete() override;
  virtual FavouritesWindowNode* AsWindowNode() override { return this; }
  virtual const FavouritesWindowNode* AsWindowNode() const override {
    return this;
  }

 private:
  const Page& folder_;
  const WindowDefinition& window_;
};

class FavouritesTreeModel : public ui::TreeNodeModel<FavouritesNode>,
                            protected Favourites::Observer {
 public:
  explicit FavouritesTreeModel(Favourites& favourites);
  ~FavouritesTreeModel();

  FavouritesRootNode& root() {
    return *reinterpret_cast<FavouritesRootNode*>(
        ui::TreeNodeModel<FavouritesNode>::root());
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
