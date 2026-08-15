#pragma once

#include "base/any_executor.h"
#include "base/lifetime.h"

#include <memory>

template <class T>
class BasicCommandRegistry;

class ControllerRegistry;
class Favourites;
class Profile;
class UiCommandRegistry;
struct GlobalCommandContext;

struct FavoritesModuleContext {
  AnyExecutor executor_;
  Profile& profile_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  ControllerRegistry& controller_registry_;
  UiCommandRegistry& ui_command_registry_;
};

class FavoritesModule : private FavoritesModuleContext {
 public:
  explicit FavoritesModule(FavoritesModuleContext&& context);
  ~FavoritesModule();

  Favourites& favourites() SCADA_LIFETIME_BOUND { return *favourites_; }

 private:
  std::unique_ptr<Favourites> favourites_;
};
