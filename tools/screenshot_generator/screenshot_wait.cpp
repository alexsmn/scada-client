#include "screenshot_wait.h"

#include "base/any_executor.h"
#include "base/thread_executor.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_service.h"

#include <gtest/gtest.h>

namespace screenshot_generator {

bool WaitForPendingNodeLoads(NodeService& node_service) {
  try {
    WaitForAwaitable(ThreadExecutor{}, WaitForPendingNodes(node_service));
    return true;
  } catch (...) {
    ADD_FAILURE() << "NodeService pending-node wait failed";
    return true;
  }
}

}  // namespace screenshot_generator
