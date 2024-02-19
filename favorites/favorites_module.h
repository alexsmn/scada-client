#pragma once

#include <memory>

class Favourites;
class Profile;

struct FavoritesModuleContext {
  Profile& profile_;
};

class FavoritesModule : private FavoritesModuleContext {
 public:
  explicit FavoritesModule(FavoritesModuleContext&& context);
  ~FavoritesModule();

  Favourites& favourites() { return *favourites_; }

 private:
  std::unique_ptr<Favourites> favourites_;
};
