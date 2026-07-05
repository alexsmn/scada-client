#pragma once

#include "profile/page.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>
#include <list>

class Favourites {
 public:
  using FolderCallback = std::function<void(const Page& folder)>;
  using FavouriteCallback =
      std::function<void(const Page& folder, const WindowDefinition& win)>;

  typedef std::list<Page> Folders;

  Favourites() {}

  const Folders& folders() const { return folders_; }

  const Page* GetFolder(std::u16string_view name = {}) const;
  const Page& GetOrAddFolder(std::u16string_view name = {});

  void Add(const WindowDefinition& win, const Page& folder);
  void Delete(const WindowDefinition& win, const Page& folder);
  void DeleteFolder(const Page& folder);

  [[nodiscard]] boost::signals2::scoped_connection SubscribeFolderAdded(
      const FolderCallback& callback) const {
    return folder_added_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeFolderDeleted(
      const FolderCallback& callback) const {
    return folder_deleted_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeFolderChanged(
      const FolderCallback& callback) const {
    return folder_changed_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeFavouriteAdded(
      const FavouriteCallback& callback) const {
    return favourite_added_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeFavouriteDeleted(
      const FavouriteCallback& callback) const {
    return favourite_deleted_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeWindowChanged(
      const FavouriteCallback& callback) const {
    return window_changed_signal_.connect(callback);
  }

  void Load(const boost::json::value& value);
  boost::json::value Save() const;

  // TODO: Move into private.
  void NotifyFolderAdded(const Page& folder) const;
  void NotifyFolderDeleted(const Page& folder) const;
  void NotifyFolderChanged(const Page& folder) const;
  void NotifyWindowAdded(const Page& folder, const WindowDefinition& win) const;
  void NotifyWindowChanged(const Page& folder,
                           const WindowDefinition& window) const;

 private:
  Folders folders_;

  mutable boost::signals2::signal<void(const Page&)> folder_added_signal_;
  mutable boost::signals2::signal<void(const Page&)> folder_deleted_signal_;
  mutable boost::signals2::signal<void(const Page&)> folder_changed_signal_;
  mutable boost::signals2::signal<void(const Page&, const WindowDefinition&)>
      favourite_added_signal_;
  mutable boost::signals2::signal<void(const Page&, const WindowDefinition&)>
      favourite_deleted_signal_;
  mutable boost::signals2::signal<void(const Page&, const WindowDefinition&)>
      window_changed_signal_;

  Favourites(const Favourites&) = delete;
  Favourites& operator=(const Favourites&) = delete;
};
