#include "screenshot_wait.h"

#include "node_service/node_promises.h"
#include "node_service/node_service.h"

#include <gtest/gtest.h>

namespace screenshot_generator {

void WaitForPromise(promise<void> promise) {
  using namespace std::chrono_literals;

  while (promise.wait_for(0ms) == promise_wait_status::timeout) {
    QApplication::processEvents(QEventLoop::WaitForMoreEvents);
  }

  promise.get();
  ProcessPostedEvents();
}

bool WaitForPendingNodeLoads(NodeService& node_service) {
  try {
    WaitForPromise(WaitForPendingNodes(node_service));
    return true;
  } catch (...) {
    ADD_FAILURE() << "NodeService pending-node wait failed";
    return true;
  }
}

}  // namespace screenshot_generator
