#pragma once

#include "base/any_executor_dispatch.h"
#include "base/cancelation.h"
#include "base/executor_util.h"
#include "base/promise.h"
#include "base/promise_executor.h"

#include <source_location>

// AnyExecutor overloads of promise_executor.h utilities.

template <class F>
inline [[nodiscard]] auto DispatchAsPromise(
    const AnyExecutor& executor,
    F&& closure,
    const std::source_location& location = std::source_location::current()) {
  using R = std::invoke_result_t<F>;
  add_promise_t<R> p;

  Dispatch(
      executor,
      [closure = std::forward<F>(closure), p]() mutable {
        if constexpr (std::is_void_v<R>) {
          std::invoke(closure);
          p.resolve();
        } else {
          ForwardPromise(std::invoke(closure), p);
        }
      },
      location);

  return p;
}

namespace internal {

template <class F>
class AnyExecutorWrappedPromiseTask {
 public:
  template <class U>
  AnyExecutorWrappedPromiseTask(const std::source_location& location,
                                AnyExecutor executor,
                                U&& func)
      : executor_{std::move(executor)},
        func_{std::forward<U>(func)},
        location_{location} {}

  template <class... Args>
  auto operator()(Args&&... args) {
    auto closure =
        [func = std::move(func_),
         args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
          return std::apply(std::move(func), std::move(args));
        };

    return DispatchAsPromise(executor_, std::move(closure), location_.get());
  }

 private:
  const AnyExecutor executor_;
  F func_;
  const DebugHolder<std::source_location> location_;
};

}  // namespace internal

template <class T>
inline auto BindPromiseExecutor(
    AnyExecutor executor,
    T&& task,
    const std::source_location& location = std::source_location::current()) {
  return internal::AnyExecutorWrappedPromiseTask<T>(
      location, std::move(executor), std::forward<T>(task));
}

template <class T, class C>
inline auto BindPromiseExecutor(
    AnyExecutor executor,
    std::weak_ptr<C> cancelation,
    T&& task,
    const std::source_location& location = std::source_location::current()) {
  return BindPromiseExecutor(
      std::move(executor),
      BindPromiseCancelation(std::move(cancelation), std::forward<T>(task),
                             location),
      location);
}

template <class T>
inline auto BindPromiseExecutor(
    AnyExecutor executor,
    const Cancelation& cancelation,
    T&& task,
    const std::source_location& location = std::source_location::current()) {
  return BindPromiseExecutor(std::move(executor), cancelation.weak_ptr(),
                             std::forward<T>(task), location);
}

inline promise<void> Delay(
    const AnyExecutor& executor,
    Duration delay,
    const std::source_location& location = std::source_location::current()) {
  promise<void> p;
  PostDelayedTask(executor, delay,
                  std::bind_front(&promise<void>::resolve, p), location);
  return p;
}
