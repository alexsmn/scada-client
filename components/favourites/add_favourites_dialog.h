#pragma once

#include "window_definition.h"

#include <string>

class DialogService;
class Favourites;

struct AddFavouritesContext {
  Favourites& favourites_;
  WindowDefinition window_def_;
};

bool ShowAddFavouritesDialog(DialogService& dialog_service,
                             AddFavouritesContext&& context);
