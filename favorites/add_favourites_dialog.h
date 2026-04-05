#pragma once

#include "base/promise.h"
#include "profile/window_definition.h"

class DialogService;
class Favourites;

struct AddFavouritesContext {
  Favourites& favourites_;
  WindowDefinition window_def_;
};

promise<> ShowAddFavouritesDialog(DialogService& dialog_service,
                                  AddFavouritesContext&& context);
