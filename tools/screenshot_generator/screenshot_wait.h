#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"

#include <QApplication>
#include <QEventLoop>

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

class NodeService;

namespace screenshot_generator {

inline void ProcessPostedEvents() {
  for (int i = 0; i < 3; ++i) {
    QApplication::processEvents(QEventLoop::AllEvents, 50);
  }
}

template <class T>
struct AwaitableResult {
  std::optional<T> value;
  std::exception_ptr error;
  bool done = false;
};

template <>
struct AwaitableResult<void> {
  std::exception_ptr error;
  bool done = false;
};

template <class T>
T WaitForAwaitable(AnyExecutor executor, Awaitable<T> awaitable) {
  auto result = std::make_shared<AwaitableResult<T>>();
  CoSpawn(executor,
          [result, awaitable = std::move(awaitable)]() mutable
              -> Awaitable<void> {
            try {
              if constexpr (std::is_void_v<T>) {
                co_await std::move(awaitable);
              } else {
                result->value.emplace(co_await std::move(awaitable));
              }
            } catch (...) {
              result->error = std::current_exception();
            }
            result->done = true;
          });

  while (!result->done) {
    QApplication::processEvents(QEventLoop::WaitForMoreEvents);
  }

  if (result->error) {
    std::rethrow_exception(result->error);
  }

  ProcessPostedEvents();
  if constexpr (!std::is_void_v<T>) {
    return std::move(*result->value);
  }
}

bool WaitForPendingNodeLoads(NodeService& node_service);

}  // namespace screenshot_generator
