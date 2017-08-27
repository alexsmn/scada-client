#pragma once

#include "ui/base/models/tree_node_model.h"
#include "services/favourites.h"

class FavouritesFolderNode;
class FavouritesWindowNode;

class FavouritesNode : public ui::TreeNode<FavouritesNode> {
 public:
  virtual void Delete() { }

  virtual FavouritesWindowNode* AsWindowNode() { return NULL; }
  virtual const FavouritesWindowNode* AsWindowNode() const { return NULL; }
};

class FavouritesRootNode : public FavouritesNode {
 public:
  explicit FavouritesRootNode(Favourites& favourites);
  
  int FindFolderNode(const Page& folder) const;
 
  // TreeNode
  virtual base::string16 GetText(int column_id) const { return base::string16(); }
};

class FavouritesFolderNode : public FavouritesNode {
 public:
  FavouritesFolderNode(Favourites& favourites, const Page& folder);
        
  int FindWindowNode(const WindowDefinition& window) const;

  // FavouritesNode
  virtual base::string16 GetText(int column_id) const { return folder_.GetTitle(); }
  virtual int GetIcon() const { return 2; }
  virtual void SetTitle(const base::string16& title);
  virtual void Delete();

  Favourites& favourites_;
  const Page& folder_;
};

class FavouritesWindowNode : public FavouritesNode {
 public:
  FavouritesWindowNode(Favourites& favourites,
                       const Page& folder,
                       const WindowDefinition& window)
      : favourites_(favourites),
        folder_(folder),
        window_(window) {
  }

  const WindowDefinition& window() const { return window_; }
 
  // FavouritesNode
  virtual base::string16 GetText(int column_id) const { return window_.GetTitle(); }
  virtual int GetIcon() const;
  virtual void SetTitle(const base::string16& title);
  virtual void Delete();
  virtual FavouritesWindowNode* AsWindowNode() { return this; }
  virtual const FavouritesWindowNode* AsWindowNode() const { return this; }

 private:
  Favourites& favourites_;
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
  virtual void OnFolderAdded(const Page& folder);
  virtual void OnFolderDeleted(const Page& folder);
  virtual void OnFolderChanged(const Page& folder);
	virtual void OnFavouriteAdded(const Page& folder, const WindowDefinition& window);
	virtual void OnFavouriteDeleted(const Page& folder, const WindowDefinition& window);
	virtual void OnWindowChanged(const Page& folder, const WindowDefinition& window);
	
 private:
  Favourites& favourites_;
};
