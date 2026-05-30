#pragma once

#include "aui/dialog_service.h"
#include "base/awaitable.h"
#include "base/any_executor.h"

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
  return u"Error";
}

template <typename T>
T RethrowResourceError(std::exception_ptr e) {
  std::rethrow_exception(e);
}

template <typename T>
inline Awaitable<T> ShowResourceErrorAsync(
    DialogService& dialog_service,
    std::u16string message,
    std::u16string title,
    std::exception_ptr e) {
  co_await dialog_service.RunMessageBox(message, title, MessageBoxMode::Error);
  if constexpr (std::is_void_v<T>) {
    std::rethrow_exception(e);
    co_return;
  } else {
    co_return RethrowResourceError<T>(e);
  }
}

template <typename T>
inline Awaitable<T> ShowResourceError(DialogService& dialog_service,
                                      std::u16string_view title,
                                      std::exception_ptr e) {
  auto message = GetResourceErrorMessage(e) + u".";
  return ShowResourceErrorAsync<T>(dialog_service, std::move(message),
                                   std::u16string{title}, e);
}

template <typename T>
inline Awaitable<T> HandleResourceErrorAsync(
    Awaitable<T> source,
    DialogService& dialog_service,
    std::u16string title) {
  std::exception_ptr error;
  try {
    if constexpr (std::is_void_v<T>) {
      co_await std::move(source);
      co_return;
    } else {
      co_return co_await std::move(source);
    }
  } catch (...) {
    error = std::current_exception();
  }

  if constexpr (std::is_void_v<T>) {
    co_await ShowResourceError<T>(dialog_service, title, error);
    co_return;
  } else {
    co_return co_await ShowResourceError<T>(dialog_service, title, error);
  }
}

template <typename T>
inline Awaitable<T> HandleResourceError(Awaitable<T> source,
                                        DialogService& dialog_service,
                                        std::u16string_view title) {
  return HandleResourceErrorAsync(std::move(source), dialog_service,
                                  std::u16string{title});
}

template <typename F>
inline auto CatchResourceError(DialogService& dialog_service,
                               std::u16string_view title,
                               F&& func) {
  return [&dialog_service, title = std::u16string{title},
          func = std::forward<decltype(func)>(func)](auto&&... args) {
    using FuncResult = std::invoke_result_t<decltype(func), decltype(args)...>;
    try {
      return HandleResourceError(func(std::forward<decltype(args)>(args)...),
                                 dialog_service, title);
    } catch (...) {
      using Result = typename FuncResult::value_type;
      return ShowResourceError<Result>(dialog_service, title,
                                       std::current_exception());
    }
  };
}
