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
                                             .title = u"Избранное",
                                             .flags = WIN_SING,
                                             .size = {200, 400}};

}

FavoritesModule::FavoritesModule(FavoritesModuleContext&& context)
    : FavoritesModuleContext{std::move(context)} {
  favourites_ = std::make_unique<Favourites>();

  controller_registry_.AddControllerFactory(
      kFavoritesWindowInfo, [](const ControllerContext& context) {
        return std::make_unique<FavouritesView>(context);
      });

  if (const auto* key = profile_.data().FindKey("favorites")) {
    favourites_->Load(*key);
  }

  profile_.RegisterSerializer([this](base::Value& data) {
    data.SetKey("favorites", favourites_->Save());
  });

  global_commands_.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_VIEW_ADD_TO_FAVOURITES}
          .set_execute_handler([this](const GlobalCommandContext& context) {
            auto* view = context.main_window.GetActiveView();
            if (!view || view->window_info().is_pane()) {
              return;
            }

            view->Save();

            auto definition = view->window_def();
            definition.title = view->GetWindowTitle();

            ShowAddFavouritesDialog(context.dialog_service,
                                    {*favourites_, std::move(definition)});
          }));
}

FavoritesModule::~FavoritesModule() {}
