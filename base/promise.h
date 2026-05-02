#pragma once

#include <D:/tc/third_party/promise.hpp/headers/promise.hpp/promise.hpp>

using namespace promise_hpp;

template <class T>
struct add_promise {
  using type = promise<T>;
};

template <class T>
struct add_promise<promise<T>> {
  using type = promise<T>;
};

template <class T>
using add_promise_t = typename add_promise<T>::type;

template <class T>
struct remove_promise {
  using type = T;
};

template <class T>
struct remove_promise<promise<T>> {
  using type = T;
};

template <class T>
using remove_promise_t = typename remove_promise<T>::type;

inline [[nodiscard]] promise<> MakeRejectedPromise() {
  return make_rejected_promise(std::exception{});
}

template <class T>
inline [[nodiscard]] promise<T> MakeRejectedPromise() {
  return make_rejected_promise<T>(std::exception{});
}

template <class T>
inline [[nodiscard]] promise<> ToVoidPromise(promise<T> promise) {
  return promise.then([](const T& value) {});
}

template <class U>
inline [[nodiscard]] promise<U> ToValuePromise(promise<> promise,
                                               const U& value) {
  return promise.then([value] { return value; });
}

template <class T>
inline [[nodiscard]] promise<> ToRejectedPromise(promise<T> promise) {
  return promise.then(
      [](const T& value) { return make_rejected_promise(std::exception{}); });
}

template <class R, class T>
inline [[nodiscard]] promise<R> ToRejectedPromise(promise<T> promise) {
  return promise.then([](const T& value) {
    return make_rejected_promise<R>(std::exception{});
  });
}

// An overload accepting a non-promise value as a source.
template <class T, class U>
inline promise<T> ForwardPromise(U&& source_value, promise<T> target_promise) {
  target_promise.resolve(std::forward<U>(source_value));
  return target_promise;
}

inline promise<> ForwardPromise(promise<> source_promise,
                                promise<> target_promise) {
  source_promise.then([target_promise]() mutable { target_promise.resolve(); })
      .except([target_promise](std::exception_ptr e) mutable {
        target_promise.reject(e);
      });
  return target_promise;
}

template <class T>
inline promise<T> ForwardPromise(promise<T> source_promise,
                                 promise<> target_promise) {
  source_promise.then([target_promise]() mutable { target_promise.resolve(); })
      .except([target_promise](std::exception_ptr e) mutable {
        target_promise.reject(e);
      });
  return target_promise;
}

template <class T>
inline promise<T> ForwardPromise(promise<T> source_promise,
                                 promise<T> target_promise) {
  source_promise
      .then([target_promise](const T& value) mutable {
        target_promise.resolve(value);
      })
      .except([target_promise](std::exception_ptr e) mutable {
        target_promise.reject(e);
      });
  return target_promise;
}

inline [[nodiscard]] promise<> FoldPromises(promise<> a, promise<> b) {
  return a.then([b]() mutable { return b; });
}

template <typename T>
inline [[nodiscard]] promise<void> IgnoreResult(promise<T> promise) {
  return promise.then([] {}, [](std::exception_ptr e) {});
}