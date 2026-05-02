#pragma once

#include "base/awaitable.h"
#include "base/executor.h"
#include "base/promise.h"
#include "net/net_executor_adapter.h"

#include <boost/asio/async_result.hpp>
#include <boost/asio/post.hpp>
#include <memory>
#include <type_traits>

template <class E, class T>
inline void StartAwaitable(E executor, promise<T> p, Awaitable<T> aw) {
  CoSpawn(
      AnyExecutor{std::move(executor)},
      [p, aw = std::move(aw)]() mutable -> Awaitable<void> {
        try {
          if constexpr (std::is_void_v<T>) {
            co_await std::move(aw);
            p.resolve();
          } else {
            p.resolve(co_await std::move(aw));
          }
        } catch (...) {
          p.reject(std::current_exception());
        }
        co_return;
      });
}

template <class E, class T>
inline promise<T> ToPromise(const E& executor, Awaitable<T> aw) {
  promise<T> p;
  StartAwaitable(executor, p, std::move(aw));
  return p;
}

template <class E>
inline promise<void> ToPromise(const E& executor, Awaitable<void> aw) {
  promise<void> p;
  StartAwaitable(executor, p, std::move(aw));
  return p;
}

template <class T>
struct PromiseAwaitableResult {
  std::exception_ptr error;
  std::shared_ptr<T> value;
};

struct PromiseAwaitableVoidResult {
  std::exception_ptr error;
};

template <class E, class T, class CompletionToken = boost::asio::use_awaitable_t<>>
inline auto PromiseToAwaitable(E executor,
                               promise<T> p,
                               CompletionToken&& token = {}) {
  auto initiate = [executor = std::move(executor), p = std::move(p)]<
                      typename Handler>(Handler&& handler) mutable {
    auto completion =
        std::make_shared<std::decay_t<Handler>>(std::forward<Handler>(handler));
    auto promise_holder = std::make_shared<promise<T>>(std::move(p));

    promise_holder
        ->then([executor, completion, promise_holder](const T& result) mutable {
          boost::asio::post(
              executor, [completion, promise_holder, result]() mutable {
                (*completion)(PromiseAwaitableResult<T>{
                    .error = std::exception_ptr{},
                    .value = std::make_shared<T>(result)});
              });
        })
        .except([executor, completion, promise_holder](
                    std::exception_ptr e) mutable {
          boost::asio::post(
              executor, [completion, promise_holder, e]() mutable {
           (*completion)(PromiseAwaitableResult<T>{
               .error = e});
         });
        });
  };

  return boost::asio::async_initiate<CompletionToken,
                                     void(PromiseAwaitableResult<T>)>(
      initiate, token);
}

template <class E, class CompletionToken = boost::asio::use_awaitable_t<>>
inline auto PromiseToAwaitable(E executor,
                               promise<void> p,
                               CompletionToken&& token = {}) {
  auto initiate = [executor = std::move(executor), p = std::move(p)]<
                      typename Handler>(Handler&& handler) mutable {
    auto completion =
        std::make_shared<std::decay_t<Handler>>(std::forward<Handler>(handler));
    auto promise_holder = std::make_shared<promise<void>>(std::move(p));

    promise_holder
        ->then([executor, completion, promise_holder]() mutable {
          boost::asio::post(
              executor, [completion, promise_holder]() mutable {
           (*completion)(PromiseAwaitableVoidResult{});
         });
        })
        .except([executor, completion, promise_holder](
                    std::exception_ptr e) mutable {
          boost::asio::post(
              executor, [completion, promise_holder, e]() mutable {
            (*completion)(PromiseAwaitableVoidResult{.error = e});
          });
        });
  };

  return boost::asio::async_initiate<CompletionToken,
                                     void(PromiseAwaitableVoidResult)>(
      initiate, token);
}

template <class E, class T>
inline Awaitable<T> AwaitPromise(E executor, promise<T> p) {
  auto result = co_await PromiseToAwaitable(std::move(executor), std::move(p));
  if (result.error)
    std::rethrow_exception(result.error);
  co_return std::move(*result.value);
}

template <class E>
inline Awaitable<void> AwaitPromise(E executor, promise<void> p) {
  auto result = co_await PromiseToAwaitable(std::move(executor), std::move(p));
  if (result.error)
    std::rethrow_exception(result.error);
  co_return;
}

template <class CompletitionHandler = boost::asio::use_awaitable_t<>>
inline auto ToAwaitable(promise<void> p, CompletitionHandler&& handler = {}) {
  auto initiate = [p]<typename Handler>(Handler&& self) mutable {
    auto promise_holder = std::make_shared<promise<void>>(std::move(p));
    promise_holder->then(
        [self = std::make_shared<Handler>(std::forward<Handler>(self)),
         promise_holder]() { (*self)(); });
  };
  return boost::asio::async_initiate<CompletitionHandler, void(/*result*/)>(
      initiate, handler);
}
