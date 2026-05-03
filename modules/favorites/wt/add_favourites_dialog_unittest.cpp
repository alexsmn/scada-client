#include "favorites/add_favourites_dialog.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "base/test/awaitable_test.h"
#include "favorites/favourites.h"

#include <gtest/gtest.h>

TEST(AddFavouritesDialogWt, RejectsUnsupportedDialog) {
  DialogServiceImplWt dialog_service;
  Favourites favourites;

  TestExecutor executor;
  auto result = StartAwaitable(
      executor,
      ShowAddFavouritesDialog(
          dialog_service, AddFavouritesContext{favourites, WindowDefinition{}}));

  EXPECT_THROW(WaitResult(executor, result), std::exception);
}
