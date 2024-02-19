#pragma once

#include <memory>

template <class T>
class BasicCommandRegistry;

class Favourites;
class Profile;
struct MainCommandContext;

struct FavoritesModuleContext {
  Profile& profile_;
  BasicCommandRegistry<MainCommandContext>& main_commands_;
};

class FavoritesModule : private FavoritesModuleContext {
 public:
  explicit FavoritesModule(FavoritesModuleContext&& context);
  ~FavoritesModule();

  Favourites& favourites() { return *favourites_; }

 private:
  std::unique_ptr<Favourites> favourites_;
};
