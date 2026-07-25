#include "configuration/objects/visible_node_model.h"

#include "aui/severity_colors.h"
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

// Stands in for a data-variable node: it carries a value quality, so the model
// paints a status dot for it.
class TestVisibleNode : public VisibleNode {
 public:
  MOCK_METHOD(std::u16string, GetText, (), (const));
  MOCK_METHOD(bool, IsBad, (), (const));
  MOCK_METHOD(bool, IsAlerting, (), (const));

  virtual bool HasQuality() const override { return true; }
};

// Stands in for a group/folder node: it has a Value column (the device state)
// but no value quality of its own.
class TestQualitylessVisibleNode : public VisibleNode {
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

// Regression: a row whose node is still resolving holds a ProxyVisibleNode
// with nothing behind it. It reports neither bad nor alerting, so the status
// dot used to fall through to the good band — painting a green "quality is
// fine" dot next to a permanently empty Value cell. Absent data gets no dot.
TEST_F(VisibleNodeModelTest, UnresolvedProxyNodeShowsNoStatusDot) {
  // The status dot only exists under the token themes; QualityColor yields
  // nothing under kLegacy, which would make both branches below look alike.
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  int value = 0;
  void* tree_node = &value;
  auto proxy_node = std::make_shared<ProxyVisibleNode>();
  model_.SetNode(tree_node, proxy_node);

  EXPECT_FALSE(model_.GetStatusColor(tree_node).has_value());

  auto underlying_node = std::make_shared<NiceMock<TestVisibleNode>>();
  ON_CALL(*underlying_node, IsBad()).WillByDefault(Return(false));
  ON_CALL(*underlying_node, IsAlerting()).WillByDefault(Return(false));
  proxy_node->SetUnderlyingNode(underlying_node);

  // Once it speaks for a real value, the good band is honest again.
  EXPECT_TRUE(model_.GetStatusColor(tree_node).has_value());

  proxy_node->SetChangeHandler(nullptr);
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kLegacy);
}

// Regression: a data group (folder) row also gets a VisibleNode — its Value
// column shows the bound device's connection state as text. That state is not
// a value quality, but the dot used to be derived from IsBad()/IsAlerting()
// for every node with a VisibleNode, so folders were painted with a
// good/bad quality dot. Only data variables get the dot.
TEST_F(VisibleNodeModelTest, GroupNodeShowsNoStatusDot) {
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  int value = 0;
  void* tree_node = &value;
  auto group_node = std::make_shared<NiceMock<TestQualitylessVisibleNode>>();
  ON_CALL(*group_node, IsBad()).WillByDefault(Return(false));
  ON_CALL(*group_node, IsAlerting()).WillByDefault(Return(false));
  model_.SetNode(tree_node, group_node);

  EXPECT_FALSE(model_.GetStatusColor(tree_node).has_value());

  // An offline device colours the Value text, but still claims no quality.
  ON_CALL(*group_node, IsBad()).WillByDefault(Return(true));
  EXPECT_FALSE(model_.GetStatusColor(tree_node).has_value());

  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kLegacy);
}

// A proxy standing in for a group must not manufacture a dot either: it
// forwards the underlying node's quality claim, and an unattached proxy has
// none.
TEST_F(VisibleNodeModelTest, ProxyOverGroupNodeShowsNoStatusDot) {
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  int value = 0;
  void* tree_node = &value;
  auto proxy_node = std::make_shared<ProxyVisibleNode>();
  model_.SetNode(tree_node, proxy_node);

  auto group_node = std::make_shared<NiceMock<TestQualitylessVisibleNode>>();
  proxy_node->SetUnderlyingNode(group_node);

  EXPECT_FALSE(model_.GetStatusColor(tree_node).has_value());

  proxy_node->SetChangeHandler(nullptr);
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kLegacy);
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
