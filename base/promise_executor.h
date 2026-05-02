#pragma once

#include "base/cancelation.h"
#include "base/executor.h"
#include "base/promise.h"

#include <source_location>

// A `Dispatch` version that returns a promise.
// `closure` can a promise, or a value, or void.
template <class F>
inline [[nodiscard]] auto DispatchAsPromise(
    Executor& executor,
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

// Always returns a `promise<R>`.
template <class F>
inline [[nodiscard]] auto InvokeAsPromise(F&& closure) {
  using R = std::invoke_result_t<F>;
  if constexpr (is_promise_v<R>) {
    promise<typename R::value_type> result_promise =
        std::invoke(std::forward<F>(closure));
    return result_promise;
  } else if constexpr (std::is_void_v<R>) {
    std::invoke(std::forward<F>(closure));
    return make_resolved_promise();
  } else {
    R result = std::invoke(std::forward<F>(closure));
    return make_resolved_promise(std::move(result));
  }
}

namespace internal {

template <class F>
class WrappedPromiseTask {
 public:
  template <class U>
  WrappedPromiseTask(const std::source_location& location,
                     std::shared_ptr<Executor> executor,
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

    return DispatchAsPromise(*executor_, std::move(closure), location_.get());
  }

 private:
  const std::shared_ptr<Executor> executor_;
  F func_;
  const DebugHolder<std::source_location> location_;
};

template <class F, class C>
struct CancelationPromiseFunc {
  // Always returns a `promise<R>`.
  template <class... Args>
  auto operator()(Args&&... args) {
    if (auto cancelation = cancelation_.lock()) {
      auto closure =
          [func = std::move(func_),
           args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            return std::apply(std::move(func), std::move(args));
          };
      return InvokeAsPromise(std::move(closure));
    } else {
      using R = std::invoke_result_t<F, Args...>;
      // TODO: Use a specific exception type.
      return make_rejected_promise<remove_promise_t<R>>(std::exception{});
    }
  }

  std::weak_ptr<C> cancelation_;
  F func_;
  DebugHolder<std::source_location> location_;
};

}  // namespace internal

template <class T, class C>
inline auto BindPromiseCancelation(
    std::weak_ptr<C> cancelation,
    T&& task,
    const std::source_location& location = std::source_location::current()) {
  return internal::CancelationPromiseFunc<T, C>{
      std::move(cancelation), std::forward<T>(task), DebugHolder{location}};
}

template <class T>
inline auto BindPromiseExecutor(
    std::shared_ptr<Executor> executor,
    T&& task,
    const std::source_location& location = std::source_location::current()) {
  return internal::WrappedPromiseTask<T>(location, std::move(executor),
                                         std::forward<T>(task));
}

template <class T, class C>
inline auto BindPromiseExecutor(
    std::shared_ptr<Executor> executor,
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
    std::shared_ptr<Executor> executor,
    const Cancelation& cancelation,
    T&& task,
    const std::source_location& location = std::source_location::current()) {
  return BindPromiseExecutor(std::move(executor), cancelation.weak_ptr(),
                             location);
}

inline promise<void> Delay(
    Executor& executor,
    Duration delay,
    const std::source_location& location = std::source_location::current()) {
  promise<void> p;
  executor.PostDelayedTask(delay, std::bind_front(&promise<void>::resolve, p),
                           location);
  return p;
}