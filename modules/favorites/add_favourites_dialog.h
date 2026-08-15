#pragma once

#include "base/awaitable.h"
#include "profile/window_definition.h"

class DialogService;
class Favourites;

struct AddFavouritesContext {
  Favourites& favourites_;
  WindowDefinition window_def_;
};

// The context is taken **by value** because this *is* a coroutine: a
// coroutine does not copy reference parameters into its frame, so an rvalue
// reference bound to a caller's temporary was already destroyed by the time
// the lazy body ran, and the favourite was saved with a title read from freed
// memory.
Awaitable<void> ShowAddFavouritesDialog(DialogService& dialog_service,
                                        AddFavouritesContext context);
