#include "configuration/objects/visible_node_model.h"

#include "base/test/test_executor.h"
#include "profile/profile.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

using namespace testing;

class VisibleNodeModelTest : public Test {
 public:
 protected:
  AnyExecutor executor_ = TestExecutor{};
  MockTimedDataService timed_data_service_;
  Profile profile_;
  VisibleNodeModel::NodeChangeHandler node_change_handler_ =
      [](void* tree_node) {};
  VisibleNodeModel model_{timed_data_service_, profile_, node_change_handler_};
};

class TestVisibleNode : public VisibleNode {
 public:
  MOCK_METHOD(std::u16string, GetText, (), (const));
  MOCK_METHOD(bool, IsBad, (), (const));
  MOCK_METHOD(bool, IsAlerting, (), (const));
};

TEST_F(VisibleNodeModelTest, Test) {
  int value = 0;
  void* tree_node = &value;
  model_.SetNode(tree_node, std::make_unique<TestVisibleNode>());
}

TEST(ProxyVisibleNodeTest, NotifiesWhenUnderlyingNodeAttached) {
  int change_count = 0;
  auto proxy_node = std::make_shared<ProxyVisibleNode>();
  proxy_node->SetChangeHandler([&] { ++change_count; });

  auto underlying_node = std::make_shared<TestVisibleNode>();
  EXPECT_CALL(*underlying_node, GetText()).WillOnce(Return(u"value"));

  proxy_node->SetUnderlyingNode(underlying_node);

  EXPECT_EQ(change_count, 1);
  EXPECT_EQ(proxy_node->GetText(), u"value");

  proxy_node->SetChangeHandler(nullptr);
}
