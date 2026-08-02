#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"

#include <functional>
#include <memory>
#include <string>

class DialogService;
class Favourites;
class FavouritesNode;

using FavouritesUrlPrompt = std::function<Awaitable<std::u16string>()>;
using FavouritesSelectedNodeProvider =
    std::function<const FavouritesNode*()>;

Awaitable<void> AddUrlToFavouritesWithPromptAsync(
    AnyExecutor executor,
    std::weak_ptr<void> lifetime_token,
    DialogService& dialog_service,
    Favourites& favourites,
    FavouritesUrlPrompt prompt_runner,
    FavouritesSelectedNodeProvider selected_node_provider);

Awaitable<void> AddUrlToFavouritesWithPrompt(
    AnyExecutor executor,
    std::weak_ptr<void> lifetime_token,
    DialogService& dialog_service,
    Favourites& favourites,
    FavouritesSelectedNodeProvider selected_node_provider);
