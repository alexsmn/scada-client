#pragma once

#include "base/strings/string16.h"

class Favourites;

bool ShowAddFavouritesDialog(Favourites& favourites,
                             base::string16& title,
                             base::string16& folder_name);
