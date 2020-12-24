#include "components/object_tree/visible_node_model.h"

#include "base/blinker.h"
#include "base/test/test_executor.h"
#include "services/profile.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

using namespace testing;

class VisibleNodeModelTest : public Test {
 public:
 protected:
  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  MockTimedDataService timed_data_service_;
  Profile profile_;
  VisibleNodeModel::NodeChangeHandler node_change_handler_ =
      [](void* tree_node) {};
  BlinkerManager blinker_manager_{executor_};
  VisibleNodeModel model_{timed_data_service_, profile_, blinker_manager_,
                          node_change_handler_};
};

TEST_F(VisibleNodeModelTest, Test) {
}
