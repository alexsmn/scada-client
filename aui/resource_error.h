#pragma once

#include "aui/dialog_service.h"
#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "base/promise.h"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

class ResourceError {
 public:
  explicit ResourceError(std::u16string message)
      : message_{std::move(message)} {}

  const std::u16string& message() const { return message_; }

 private:
  const std::u16string message_;
};

inline std::u16string GetResourceErrorMessage(std::exception_ptr e) {
  try {
    std::rethrow_exception(e);
  } catch (const ResourceError& e) {
    return e.message();
  } catch (...) {
    assert(false);
  }
  // Reached for anything that is not a `ResourceError`. The trailing
  // return also satisfies cppcheck's `missingReturn` checker, which does
  // not treat `std::rethrow_exception` as terminating control flow.
  return u"Œ¯Ë·Í‡";
}

template <typename T>
T RethrowResourceError(std::exception_ptr e) {
  std::rethrow_exception(e);
}

template <typename T>
inline Awaitable<T> ShowResourceErrorAsync(
    promise<MessageBoxResult> message_box_promise,
    std::exception_ptr e) {
  auto executor = co_await boost::asio::this_coro::executor;
  co_await AwaitPromise(executor, std::move(message_box_promise));
  if constexpr (std::is_void_v<T>) {
    std::rethrow_exception(e);
    co_return;
  } else {
    co_return RethrowResourceError<T>(e);
  }
}

template <typename T>
inline promise<T> ShowResourceError(DialogService& dialog_service,
                                    std::u16string_view title,
                                    std::exception_ptr e) {
  auto message = GetResourceErrorMessage(e) + u".";
  auto message_box_promise =
      dialog_service.RunMessageBox(message, title, MessageBoxMode::Error);
  return ToPromise(MakeThreadAnyExecutor(),
                   ShowResourceErrorAsync<T>(std::move(message_box_promise),
                                             e));
}

template <typename T>
inline Awaitable<T> HandleResourceErrorAsync(
    promise<T> source_promise,
    DialogService& dialog_service,
    std::u16string title) {
  auto executor = co_await boost::asio::this_coro::executor;
  std::exception_ptr error;
  try {
    if constexpr (std::is_void_v<T>) {
      co_await AwaitPromise(executor, std::move(source_promise));
      co_return;
    } else {
      co_return co_await AwaitPromise(executor, std::move(source_promise));
    }
  } catch (...) {
    error = std::current_exception();
  }

  if constexpr (std::is_void_v<T>) {
    co_await AwaitPromise(executor,
                          ShowResourceError<T>(dialog_service, title, error));
    co_return;
  } else {
    co_return co_await AwaitPromise(
        executor, ShowResourceError<T>(dialog_service, title, error));
  }
}

template <typename T>
inline promise<T> HandleResourceError(promise<T> source_promise,
                                      DialogService& dialog_service,
                                      std::u16string_view title) {
  return ToPromise(
      MakeThreadAnyExecutor(),
      HandleResourceErrorAsync(std::move(source_promise), dialog_service,
                               std::u16string{title}));
}

template <typename F>
inline auto CatchResourceError(DialogService& dialog_service,
                               std::u16string_view title,
                               F&& func) {
  return [&dialog_service, title = std::u16string{title},
          func = std::forward<decltype(func)>(func)](auto&&... args) {
    using FuncResult = std::invoke_result_t<decltype(func), decltype(args)...>;
    try {
      auto p = func(std::forward<decltype(args)>(args)...);
      if constexpr (is_promise_v<FuncResult>) {
        return HandleResourceError(std::move(p), dialog_service, title);
      } else {
        return make_resolved_promise(std::move(p));
      }
    } catch (...) {
      using Result =
          std::conditional_t<is_promise_v<FuncResult>,
                             remove_promise_t<FuncResult>, FuncResult>;
      return ShowResourceError<Result>(dialog_service, title,
                                       std::current_exception());
    }
  };
}
