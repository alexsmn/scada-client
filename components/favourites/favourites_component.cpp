#include "components/favourites/favourites_view.h"
#include "controller_registry.h"

const WindowInfo kWindowInfo = {ID_FAVOURITES_VIEW, "Favorites", u"Избранное",
                                WIN_SING,           200,         400};

REGISTER_CONTROLLER(FavouritesView, kWindowInfo);
