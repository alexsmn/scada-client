#pragma once

#include <string_view>

class Favourites;
class FavouritesNode;

enum class AddFavouriteUrlResult {
  Added,
  InvalidUrl,
};

AddFavouriteUrlResult AddUrlToFavourites(
    Favourites& favourites,
    const FavouritesNode* selected_node,
    std::u16string_view url);
