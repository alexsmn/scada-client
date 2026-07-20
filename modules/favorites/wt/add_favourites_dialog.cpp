#include "favorites/add_favourites_dialog.h"

#include "aui/wt/dialog_stub.h"

Awaitable<void> ShowAddFavouritesDialog(DialogService& dialog_service,
                                        AddFavouritesContext&& context) {
  return scada::aui::wt::MakeUnsupportedDialogAwaitable<void>();
}
