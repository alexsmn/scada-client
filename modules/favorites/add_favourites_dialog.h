#pragma once

#include "base/awaitable.h"
#include "profile/window_definition.h"

class DialogService;
class Favourites;

struct AddFavouritesContext {
  Favourites& favourites_;
  WindowDefinition window_def_;
};

Awaitable<void> ShowAddFavouritesDialog(DialogService& dialog_service,
                                        AddFavouritesContext&& context);
