#include "favorites_module.h"

#include "favorites/favourites.h"
#include "profile/profile.h"

FavoritesModule::FavoritesModule(FavoritesModuleContext&& context)
    : FavoritesModuleContext{std::move(context)} {
  favourites_ = std::make_unique<Favourites>();

  if (const auto* key = profile_.data().FindKey("favorites")) {
    favourites_->Load(*key);
  }

  profile_.RegisterSerializer([this](base::Value& data) {
    data.SetKey("favorites", favourites_->Save());
  });
}

FavoritesModule::~FavoritesModule() {}
