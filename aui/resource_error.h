#pragma once

#include "aui/dialog_service.h"
#include "base/promise.h"

#include <stdexcept>
#include <string>

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
  return u"Ошибка";
}

template <typename T>
inline promise<T> ShowResourceError(DialogService& dialog_service,
                                    std::u16string_view title,
                                    std::exception_ptr e) {
  auto message = GetResourceErrorMessage(e) + u".";
  return dialog_service.RunMessageBox(message, title, MessageBoxMode::Error)
      .then([e](MessageBoxResult) { return make_rejected_promise<T>(e); });
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
        return p.except([&dialog_service, title](std::exception_ptr e) {
          return ShowResourceError<remove_promise_t<FuncResult>>(dialog_service,
                                                                 title, e);
        });
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
