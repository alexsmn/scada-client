#pragma once

#include "screenshot_generator_ns_compat.h"

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/check.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

class NodeService;

namespace scada::screenshot_generator {

inline void ProcessPostedEvents() {
  for (int i = 0; i < 3; ++i) {
    QApplication::processEvents(QEventLoop::AllEvents, 50);
  }
}

// Runs a real event loop for `duration`. Unlike bare processEvents() spins,
// QEventLoop::exec keeps firing QTimers on a drained queue (on macOS the
// processEvents forms never do), so MessageLoopQt-scheduled continuations —
// async model-row inserts included — actually run before capture.
inline void PumpEventLoopFor(std::chrono::milliseconds duration) {
  QEventLoop loop;
  QTimer::singleShot(duration, &loop, &QEventLoop::quit);
  loop.exec();
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
  CoSpawn(
      executor,
      [result, awaitable = std::move(awaitable)]() mutable -> Awaitable<void> {
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

  // Fail-stop instead of hanging the build: a livelocked fetch pipeline
  // (e.g. node-model eviction churn) otherwise leaves the POST_BUILD step
  // stuck forever with no diagnostic.
  QElapsedTimer elapsed;
  elapsed.start();
  while (!result->done) {
    QApplication::processEvents(QEventLoop::WaitForMoreEvents);
    base::Check(!elapsed.hasExpired(180'000),
                "WaitForAwaitable: 180s deadline exceeded; async work never "
                "completed (livelock?)");
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

}  // namespace scada::screenshot_generator
