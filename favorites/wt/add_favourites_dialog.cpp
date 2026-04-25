#include "favorites/add_favourites_dialog.h"

#include "aui/wt/dialog_stub.h"

promise<> ShowAddFavouritesDialog(DialogService& dialog_service,
                                  AddFavouritesContext&& context) {
  return aui::wt::MakeUnsupportedDialogPromise<void>();
}
