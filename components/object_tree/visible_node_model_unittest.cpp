#include "components/object_tree/visible_node_model.h"

#include "base/blinker.h"
#include "base/test/test_executor.h"
#include "services/profile.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(VisibleNodeModel, Test) {
  auto executor = std::make_shared<TestExecutor>();
  MockTimedDataService timed_data_service;
  Profile profile;
  VisibleNodeModel::NodeChangeHandler node_change_handler =
      [](void* tree_node) {};
  BlinkerManager blinker_manager{executor};
  VisibleNodeModel model{timed_data_service, profile, blinker_manager,
                         node_change_handler};
}
