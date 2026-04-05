#include "favorites/favorites_module.h"

#include "common_resources.h"
#include "controller/command_registry.h"
#include "controller/controller_registry.h"
#include "controller/window_info.h"
#include "core/global_command_context.h"
#include "favorites/add_favourites_dialog.h"
#include "favorites/favourites.h"
#include "favorites/favourites_view.h"
#include "main_window/main_window.h"
#include "profile/profile.h"

namespace {

constexpr WindowInfo kFavoritesWindowInfo = {.command_id = ID_FAVOURITES_VIEW,
                                             .name = "Favorites",
                                             .title = u"���������",
                                             .flags = WIN_SING,
                                             .size = {200, 400}};

}

FavoritesModule::FavoritesModule(FavoritesModuleContext&& context)
    : FavoritesModuleContext{std::move(context)} {
  favourites_ = std::make_unique<Favourites>();

  controller_registry_.AddControllerFactory(
      kFavoritesWindowInfo,
      [&favorites = *favourites_](const ControllerContext& context) {
        return std::make_unique<FavouritesView>(context, favorites);
      });

  if (auto* key = profile_.data().is_object() ? profile_.data().as_object().if_contains("favorites") : nullptr) {
    favourites_->Load(*key);
  }

  profile_.RegisterSerializer([this](boost::json::value& data) {
    data.as_object()["favorites"] = favourites_->Save();
  });

  global_commands_.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_VIEW_ADD_TO_FAVOURITES}
          .set_execute_handler([this](const GlobalCommandContext& context) {
            auto* view = context.main_window.GetActiveView();
            if (!view || view->GetWindowInfo().is_pane()) {
              return;
            }

            auto definition = view->Save();
            definition.title = view->GetWindowTitle();

            ShowAddFavouritesDialog(context.dialog_service,
                                    {*favourites_, std::move(definition)});
          }));
}

FavoritesModule::~FavoritesModule() {}
