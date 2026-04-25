#include "favorites/favourites_url.h"

#include "components/web/web_component.h"
#include "favorites/favourites.h"
#include "favorites/favourites_tree_model.h"

namespace {

bool IsWebUrl(std::u16string_view url) {
  return url.starts_with(u"http://") || url.starts_with(u"https://");
}

}  // namespace

AddFavouriteUrlResult AddUrlToFavourites(Favourites& favourites,
                                         const FavouritesNode* selected_node,
                                         std::u16string_view url) {
  if (!IsWebUrl(url)) {
    return AddFavouriteUrlResult::InvalidUrl;
  }

  const FavouritesNode* node = selected_node;
  if (node && !node->AsFolderNode()) {
    node = node->parent();
  }

  const Page& folder = node && node->AsFolderNode()
                           ? node->AsFolderNode()->folder()
                           : favourites.GetOrAddFolder();

  WindowDefinition win{kWebWindowInfo};
  win.path = url;
  favourites.Add(win, folder);

  return AddFavouriteUrlResult::Added;
}
