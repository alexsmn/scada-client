#pragma once

#include "base/promise.h"

#include <QApplication>
#include <QEventLoop>

#include <chrono>
#include <utility>

class NodeService;

namespace screenshot_generator {

inline void ProcessPostedEvents() {
  for (int i = 0; i < 3; ++i) {
    QApplication::processEvents(QEventLoop::AllEvents, 50);
  }
}

template <class T>
T WaitForPromise(promise<T> promise) {
  using namespace std::chrono_literals;

  while (promise.wait_for(0ms) == promise_wait_status::timeout) {
    QApplication::processEvents(QEventLoop::WaitForMoreEvents);
  }

  auto result = promise.get();
  ProcessPostedEvents();
  return std::move(result);
}

void WaitForPromise(promise<void> promise);

bool WaitForPendingNodeLoads(NodeService& node_service);

}  // namespace screenshot_generator
