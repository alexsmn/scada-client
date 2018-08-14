#pragma once

#include "base/strings/string16.h"
#include "window_definition.h"

class DialogService;
class Favourites;

struct AddFavouritesContext {
  Favourites& favourites_;
  WindowDefinition window_def_;
};

bool ShowAddFavouritesDialog(DialogService& dialog_service,
                             AddFavouritesContext&& context);
