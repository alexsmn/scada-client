#pragma once

#include "aui/dialog_service.h"
#include "base/any_executor.h"
#include "base/awaitable.h"

#include <string>
#include <utility>

// Fire-and-forget message box, for callers that are not coroutines.
//
// `DialogService::RunMessageBox` returns a *lazy* `Awaitable`: the coroutine
// body does not start until something awaits it. A bare call therefore
// constructs the awaitable, discards it, and shows no box at all — silently,
// with no diagnostic. That defect has now shipped repeatedly (the limits
// write path, ID_ITEM_ENABLE/ID_ITEM_DISABLE, favourites, Help→About, and the
// seven sites this helper was introduced to fix), always in the same shape: a
// plain `void` function that wants to tell the operator something and has no
// `co_await` available to it.
//
// Use this from any non-coroutine context. From inside a coroutine, prefer
// `co_await dialog_service.RunMessageBox(...)` directly — that both shows the
// box and lets you read the operator's answer, which this deliberately cannot
// do (the result is discarded along with the spawned coroutine).
//
// The arguments are taken by value and moved into the spawned coroutine.
// `RunMessageBox` takes `std::u16string_view`, and the box outlives the call
// that requested it, so a view onto a caller-owned temporary would dangle.
namespace detail {

inline Awaitable<void> ShowMessageBoxAsync(DialogService& dialog_service,
                                           std::u16string message,
                                           std::u16string title,
                                           MessageBoxMode mode) {
  try {
    co_await dialog_service.RunMessageBox(message, title, mode);
  } catch (...) {
    // A message box that fails to run must not propagate out of a detached
    // coroutine: `boost::asio::detached` calls `std::terminate` on an escaped
    // exception. Cancellation during shutdown is the expected case.
  }
  co_return;
}

}  // namespace detail

// `dialog_service` must outlive the spawned coroutine. Every current caller
// holds it as a module- or view-scoped reference, which does; a dialog-owned
// service would not (see `LimitModel::WriteLimits` for the companion trap of
// capturing `this` from a dialog-owned model).
inline void ShowMessageBox(AnyExecutor executor,
                           DialogService& dialog_service,
                           std::u16string message,
                           std::u16string title,
                           MessageBoxMode mode) {
  CoSpawn(std::move(executor), [&dialog_service, message = std::move(message),
                                title = std::move(title), mode]() mutable {
    return detail::ShowMessageBoxAsync(dialog_service, std::move(message),
                                       std::move(title), mode);
  });
}
