#pragma once

#include "base/observer_list.h"
#include "services/page.h"

#include <list>

class Favourites {
 public:
  struct Observer {
    virtual void OnFolderAdded(const Page& folder) {}
    virtual void OnFolderDeleted(const Page& folder) {}
    virtual void OnFolderChanged(const Page& folder) {}
    virtual void OnFavouriteAdded(const Page& folder,
                                  const WindowDefinition& win) {}
    virtual void OnFavouriteDeleted(const Page& folder,
                                    const WindowDefinition& win) {}
    virtual void OnWindowChanged(const Page& folder,
                                 const WindowDefinition& window) {}
  };

  typedef std::list<Page> Folders;

  Favourites() {}

  const Folders& folders() const { return folders_; }

  const Page* GetFolder(std::u16string_view name = {}) const;
  const Page& GetOrAddFolder(std::u16string_view name = {});

  void Add(const WindowDefinition& win, const Page& folder);
  void Delete(const WindowDefinition& win, const Page& folder);
  void DeleteFolder(const Page& folder);

  void AddObserver(Observer& observer) { observers_.AddObserver(&observer); }
  void RemoveObserver(Observer& observer) {
    observers_.RemoveObserver(&observer);
  }

  void Load(const base::Value& value);
  base::Value Save() const;

  // TODO: Move into private.
  void NotifyFolderAdded(const Page& folder) const;
  void NotifyFolderDeleted(const Page& folder) const;
  void NotifyFolderChanged(const Page& folder) const;
  void NotifyWindowAdded(const Page& folder, const WindowDefinition& win) const;
  void NotifyWindowChanged(const Page& folder,
                           const WindowDefinition& window) const;

 private:
  Folders folders_;

  mutable base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(Favourites);
};
