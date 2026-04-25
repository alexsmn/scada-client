#include "favorites/favourites_add_url.h"

#include "aui/dialog_service.h"
#include "aui/prompt_dialog.h"
#include "aui/translation.h"
#include "base/awaitable_promise.h"
#include "favorites/favourites.h"
#include "favorites/favourites_url.h"
#include "net/net_executor_adapter.h"

namespace {

const char16_t kAddUrl[] = u"Add Web Page";

}  // namespace

Awaitable<void> AddUrlToFavouritesWithPromptAsync(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<void> lifetime_token,
    DialogService& dialog_service,
    Favourites& favourites,
    FavouritesUrlPrompt prompt_runner,
    FavouritesSelectedNodeProvider selected_node_provider) {
  auto url = co_await AwaitPromise(NetExecutorAdapter{executor},
                                   prompt_runner());

  if (lifetime_token.expired())
    co_return;

  const auto* selected_node = selected_node_provider();
  if (AddUrlToFavourites(favourites, selected_node, url) ==
      AddFavouriteUrlResult::Added) {
    co_return;
  }

  if (lifetime_token.expired())
    co_return;

  co_await AwaitPromise(
      NetExecutorAdapter{std::move(executor)},
      dialog_service.RunMessageBox(
          Translate("A valid URL must start with \"http://\" or "
                    "\"https://\"."),
          kAddUrl, MessageBoxMode::Error));
  throw std::exception{};
}

promise<> AddUrlToFavouritesWithPrompt(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<void> lifetime_token,
    DialogService& dialog_service,
    Favourites& favourites,
    std::function<const FavouritesNode*()> selected_node_provider) {
  return ToPromise(NetExecutorAdapter{executor},
                   AddUrlToFavouritesWithPromptAsync(
                       executor, std::move(lifetime_token), dialog_service,
                       favourites,
                       [&dialog_service] {
                         return RunPromptDialog(dialog_service,
                                                Translate("URL:"), kAddUrl);
                       },
                       std::move(selected_node_provider)));
}
