#include "favorites/add_favourites_dialog.h"

promise<> ShowAddFavouritesDialog(DialogService& dialog_service,
                                  AddFavouritesContext&& context) {
  return MakeRejectedPromise();
}
