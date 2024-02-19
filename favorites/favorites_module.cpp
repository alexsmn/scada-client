#include "favorites_module.h"

#include "common_resources.h"
#include "controller/command_registry.h"
#include "controller/window_info.h"
#include "favorites/add_favourites_dialog.h"
#include "favorites/favourites.h"
#include "core/main_command_context.h"
#include "main_window/main_window.h"
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

  main_commands_.AddCommand(
      BasicCommand<MainCommandContext>{ID_VIEW_ADD_TO_FAVOURITES}
          .set_execute_handler([this](const MainCommandContext& context) {
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
