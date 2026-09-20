#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/check.h"
#include "scada/node_id.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

class NodeService;
class TimedDataService;

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

// Spawns `awaitable` onto `executor` and hands back the slot its outcome will
// land in, WITHOUT waiting for it.
//
// This is the half of WaitForAwaitable that a dialog capture can use. A dialog
// awaitable does not complete until the dialog is dismissed, and the capture
// has to grab the dialog while it is still up — so blocking here would
// deadlock. The capture starts the awaitable, grabs and rejects the dialog,
// and only then waits on the returned handle.
template <class T>
std::shared_ptr<AwaitableResult<T>> StartAwaitable(AnyExecutor executor,
                                                   Awaitable<T> awaitable) {
  auto result = std::make_shared<AwaitableResult<T>>();
  CoSpawn(
      std::move(executor),
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
  return result;
}

// Rethrows whatever `result` caught, if anything. Separate from the wait so a
// caller that expects the awaitable to end in an exception - a modal dismissed
// through reject(), which is how every dialog capture ends - can decide for
// itself whether that is a failure.
template <class T>
void RethrowAwaitableError(const std::shared_ptr<AwaitableResult<T>>& result) {
  if (result->error) {
    std::rethrow_exception(result->error);
  }
}

template <class T>
T WaitForAwaitable(AnyExecutor executor, Awaitable<T> awaitable) {
  auto result = StartAwaitable(std::move(executor), std::move(awaitable));

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

  RethrowAwaitableError(result);

  ProcessPostedEvents();
  if constexpr (!std::is_void_v<T>) {
    return std::move(*result->value);
  }
}

// `executor` must be the generator's own (Qt-pumped) executor. These used to
// initiate on a private ThreadExecutor, so the wait ran on a worker while the
// GUI thread pumped: PendingNodesWaiter reads GetPendingTaskCount() and
// subscribes on the executor-affine NodeService, and the completion flag was
// a plain bool written on one thread and spun on from the other.
bool WaitForPendingNodeLoads(AnyExecutor executor, NodeService& node_service);

// Pumps the event loop until nothing is still loading: no node fetch is in
// flight and no timed data is waiting on the history it asked for. The two are
// sequential — an alias resolves via a node fetch, and only then starts its
// history read — so this alternates between them until both are quiet.
//
// Use this instead of pumping for a fixed duration. A fixed pump is a shared
// budget, so a run capturing many windows settles each one less than a
// single-window run does, and the same spec renders with or without its
// per-row trends depending on what else the run contained. Nothing failed when
// that happened; the incomplete image just shipped.
//
// Returns false if the deadline expires with work still outstanding (reported
// as a test failure).
bool WaitForPendingData(AnyExecutor executor,
                        NodeService& node_service,
                        TimedDataService& timed_data_service);

// Makes each node in `node_ids` fully resident — its own attributes, its
// hierarchical children (the analog property bands), its type definition (so
// `node[aggregate_declaration_id]` resolves), and every property child's value
// — then waits for those fetches to settle. Standalone captures (the graph
// widget, the series inspector, the write/limits dialogs) are built outside
// the main-window/tree flow that would otherwise pull these property children
// resident, and TimedData only fetches the node itself (NodeOnly), so without
// this `node[...].value()` reads (EU range, engineering units, limit bands)
// come back empty. Null ids are skipped; returns false only when a non-null
// id is unknown to the service.
bool FetchNodesResident(AnyExecutor executor,
                        NodeService& node_service,
                        std::span<const scada::NodeId> node_ids);

}  // namespace scada::screenshot_generator
