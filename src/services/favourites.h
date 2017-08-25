#pragma once

#include "base/observer_list.h"
#include "client/services/page.h"

#include <list>

class Favourites {
 public:	
	struct Observer	{
		virtual void OnFolderAdded(const Page& folder) { }
		virtual void OnFolderDeleted(const Page& folder) { }
		virtual void OnFolderChanged(const Page& folder) { }
		virtual void OnFavouriteAdded(const Page& folder, const WindowDefinition& win) { }
		virtual void OnFavouriteDeleted(const Page& folder, const WindowDefinition& win) { }
		virtual void OnWindowChanged(const Page& folder, const WindowDefinition& window) { }
	};

	typedef std::list<Page>	Folders;

  Favourites() {}

	const Folders& folders() const { return folders_; }
	
	const Page* GetFolder(const base::char16* folder = NULL) const;
	const Page& GetOrAddFolder(const base::char16* name = NULL);

	void Add(const WindowDefinition& win, const Page& folder);
	void Delete(const WindowDefinition& win, const Page& folder);
	void DeleteFolder(const Page& folder);

	void AddObserver(Observer& observer) { observers_.AddObserver(&observer); }
	void RemoveObserver(Observer& observer) { observers_.RemoveObserver(&observer); }

	void Load(const xml::Node& root_node);
	void Save(xml::Node& root_node) const;
	
  // TODO: Move into private.
	void NotifyFolderAdded(const Page& folder) const;
	void NotifyFolderDeleted(const Page& folder) const;
	void NotifyFolderChanged(const Page& folder) const;
	void NotifyWindowAdded(const Page& folder, const WindowDefinition& win) const;
	void NotifyWindowChanged(const Page& folder, const WindowDefinition& window) const;

 private:
	Folders folders_;

	mutable base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(Favourites);
};
