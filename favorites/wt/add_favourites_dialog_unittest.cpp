#include "favorites/add_favourites_dialog.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "favorites/favourites.h"

#include <gtest/gtest.h>

TEST(AddFavouritesDialogWt, RejectsUnsupportedDialog) {
  DialogServiceImplWt dialog_service;
  Favourites favourites;

  auto promise = ShowAddFavouritesDialog(
      dialog_service, AddFavouritesContext{favourites, WindowDefinition{}});

  EXPECT_THROW(promise.get(), std::exception);
}
