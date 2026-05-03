#pragma once

#include <memory>

class ActionManager;
class ControllerRegistry;
class Favourites;
class Profile;
struct GlobalCommandContext;

struct FavoritesModuleContext {
  Profile& profile_;
  ActionManager& action_manager_;
  ControllerRegistry& controller_registry_;
};

class FavoritesModule : private FavoritesModuleContext {
 public:
  explicit FavoritesModule(FavoritesModuleContext&& context);
  ~FavoritesModule();

  Favourites& favourites() { return *favourites_; }

 private:
  std::unique_ptr<Favourites> favourites_;
};
