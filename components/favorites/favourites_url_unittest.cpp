#include "favorites/favourites_url.h"

#include "components/web/web_component.h"
#include "favorites/favourites.h"
#include "favorites/favourites_tree_model.h"

#include <gtest/gtest.h>

namespace {

const WindowDefinition& OnlyWindow(const Page& folder) {
  EXPECT_EQ(folder.GetWindowCount(), 1);
  return folder.GetWindow(0);
}

}  // namespace

TEST(FavouritesUrlTest, InvalidUrlDoesNotCreateFolder) {
  Favourites favourites;

  EXPECT_EQ(AddUrlToFavourites(favourites, /*selected_node=*/nullptr,
                               u"ftp://example.com"),
            AddFavouriteUrlResult::InvalidUrl);

  EXPECT_TRUE(favourites.folders().empty());
}

TEST(FavouritesUrlTest, ValidUrlWithoutSelectionAddsToDefaultFolder) {
  Favourites favourites;

  EXPECT_EQ(AddUrlToFavourites(favourites, /*selected_node=*/nullptr,
                               u"https://example.com"),
            AddFavouriteUrlResult::Added);

  ASSERT_EQ(favourites.folders().size(), 1);
  const auto& folder = favourites.folders().front();
  const auto& window = OnlyWindow(folder);
  EXPECT_EQ(window.type, kWebWindowInfo.name);
  EXPECT_EQ(window.path, std::filesystem::path{u"https://example.com"});
}

TEST(FavouritesUrlTest, ValidUrlWithFolderSelectionAddsToSelectedFolder) {
  Favourites favourites;
  const Page& first = favourites.GetOrAddFolder(u"First");
  const Page& second = favourites.GetOrAddFolder(u"Second");
  FavouritesTreeModel model{favourites};
  const auto& selected_folder =
      static_cast<const FavouritesFolderNode&>(model.root().GetChild(1));

  EXPECT_EQ(AddUrlToFavourites(favourites, &selected_folder,
                               u"http://example.com"),
            AddFavouriteUrlResult::Added);

  EXPECT_EQ(first.GetWindowCount(), 0);
  ASSERT_EQ(second.GetWindowCount(), 1);
  EXPECT_EQ(second.GetWindow(0).path, std::filesystem::path{u"http://example.com"});
}

TEST(FavouritesUrlTest, ValidUrlWithWindowSelectionAddsToParentFolder) {
  Favourites favourites;
  const Page& folder = favourites.GetOrAddFolder(u"Folder");
  WindowDefinition existing{kWebWindowInfo};
  existing.path = std::filesystem::path{u"https://existing.example.com"};
  favourites.Add(existing, folder);
  FavouritesFolderNode selected_folder{favourites, folder};
  selected_folder.Add(0, std::make_unique<FavouritesWindowNode>(
                             favourites, folder, kWebWindowInfo,
                             folder.GetWindow(0)));
  const auto& selected_window =
      static_cast<const FavouritesWindowNode&>(selected_folder.GetChild(0));

  EXPECT_EQ(AddUrlToFavourites(favourites, &selected_window,
                               u"https://new.example.com"),
            AddFavouriteUrlResult::Added);

  ASSERT_EQ(folder.GetWindowCount(), 2);
  EXPECT_EQ(folder.GetWindow(0).path,
            std::filesystem::path{u"https://existing.example.com"});
  EXPECT_EQ(folder.GetWindow(1).path,
            std::filesystem::path{u"https://new.example.com"});
}
