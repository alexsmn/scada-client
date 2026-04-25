#pragma once

#include "base/awaitable.h"
#include "base/promise.h"

#include <functional>
#include <memory>
#include <string>

class DialogService;
class Executor;
class Favourites;
class FavouritesNode;

using FavouritesUrlPrompt = std::function<promise<std::u16string>()>;
using FavouritesSelectedNodeProvider =
    std::function<const FavouritesNode*()>;

Awaitable<void> AddUrlToFavouritesWithPromptAsync(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<void> lifetime_token,
    DialogService& dialog_service,
    Favourites& favourites,
    FavouritesUrlPrompt prompt_runner,
    FavouritesSelectedNodeProvider selected_node_provider);

promise<> AddUrlToFavouritesWithPrompt(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<void> lifetime_token,
    DialogService& dialog_service,
    Favourites& favourites,
    FavouritesSelectedNodeProvider selected_node_provider);
