#include "configuration/tree/configuration_tree_view.h"

#include "address_space/test/test_scada_node_states.h"
#include "aui/qt/tree.h"
#include "aui/test/app_environment.h"
#include "configuration/tree/configuration_tree_drop_handler.h"
#include "configuration/tree/configuration_tree_model.h"
#include "configuration/tree/configuration_tree_node.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "controller/test/controller_environment.h"
#include "model/data_items_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "profile/window_definition.h"

#include <gmock/gmock.h>

#include <memory>
#include <string>
#include <vector>

using namespace testing;

namespace {

// Exposes the two protected accessors. Both are what the view itself uses.
class TestConfigurationTreeView : public ConfigurationTreeView {
 public:
  using ConfigurationTreeView::ConfigurationTreeView;
  using ConfigurationTreeView::model;
  using ConfigurationTreeView::tree_view;
};

}  // namespace

// The Explorer orders one parent's rows for reading: objects above variables,
// and each run by display name.
//
// The regression is the key that used to sit between those two. Siblings were
// grouped by their type definition's *NodeId*, an integer that appears nowhere
// on screen, so the groups came out in an order no reader could account for —
// in the shipped hardware-tree capture `Канал MODBUS` (type SCADA.108) led and
// `Канал МЭК-104` (type SCADA.124) sat fourth, with two unrelated rows between
// them. The fixture below reproduces exactly that shape: the type ids ascend in
// the reverse of the names, so a comparator that still consults them fails
// while one that does not passes.
//
// The rule had been in the comparator since before 2023 and had never run: the
// handler was installed after `SetSorted(true)`, so Qt's own `lessThan` sorted
// by DisplayRole instead (349beab01 / visual_review V43). Fixing the
// installation order is what first made it visible.
class ConfigurationTreeViewSortTest : public Test {
 protected:
  void SetUp() override {
    node_service_.AddAll(GetScadaNodeStates());

    // Three object types, so each row below can carry its own. Their ids
    // ascend; the names of their instances descend.
    for (const scada::NodeId& type_id :
         {kFirstTypeId, kSecondTypeId, kThirdTypeId}) {
      node_service_.Add(
          scada::NodeState{.node_id = type_id,
                           .node_class = scada::NodeClass::ObjectType,
                           .parent_id = scada::id::BaseObjectType,
                           .reference_type_id = scada::id::HasSubtype});
    }

    // Objects, in browse order, named so that neither the browse order nor the
    // type-id order is the answer.
    AddChild(scada::NodeId{3001, 1}, scada::NodeClass::Object, kFirstTypeId,
             u"Яблоко");
    AddChild(scada::NodeId{3002, 1}, scada::NodeClass::Object, kSecondTypeId,
             u"Берёза");
    AddChild(scada::NodeId{3003, 1}, scada::NodeClass::Object, kThirdTypeId,
             u"Астра");

    // A variable whose name sorts ahead of every object above, so "objects
    // first" is doing work rather than agreeing with the name order.
    AddChild(scada::NodeId{3004, 1}, scada::NodeClass::Variable,
             scada::data_items::id::DataItemType, u"Аметист");

    view_ = std::make_unique<TestConfigurationTreeView>(
        MakeContext(), MakeModel(), MakeDropHandler());
    ui_view_ = view_->Init(WindowDefinition{});
    ASSERT_THAT(ui_view_, NotNull());
  }

  void AddChild(const scada::NodeId& node_id,
                scada::NodeClass node_class,
                const scada::NodeId& type_definition_id,
                std::u16string_view display_name) {
    node_service_.Add(scada::NodeState{
        .node_id = node_id,
        .node_class = node_class,
        .type_definition_id = type_definition_id,
        .parent_id = scada::data_items::id::DataItems,
        .reference_type_id = scada::id::Organizes,
        .attributes = scada::NodeAttributes{
            .display_name = scada::LocalizedText{display_name}}});
  }

  ControllerContext MakeContext() {
    ControllerContext context = env_.MakeControllerContext();
    context.node_service_ = node_service_;
    return context;
  }

  std::shared_ptr<ConfigurationTreeModel> MakeModel() {
    auto model =
        std::make_shared<ConfigurationTreeModel>(ConfigurationTreeModelContext{
            env_.executor_,
            std::make_unique<NodeServiceTreeImpl>(NodeServiceTreeImplContext{
                .executor_ = env_.executor_,
                .node_service_ = node_service_,
                .root_node_ =
                    node_service_.GetNode(scada::data_items::id::DataItems),
                .reference_filter_ = {{scada::id::Organizes, true}}})});
    model->Init();
    return model;
  }

  std::unique_ptr<ConfigurationTreeDropHandler> MakeDropHandler() {
    return std::make_unique<ConfigurationTreeDropHandler>(
        ConfigurationTreeDropHandlerContext{env_.executor_, node_service_,
                                            env_.task_manager_,
                                            env_.create_tree_});
  }

  // The root's child rows in the order the view shows them.
  std::vector<std::u16string> VisibleRows() {
    ConfigurationTreeNode* root = view_->model().root();
    EXPECT_THAT(root, NotNull());
    if (!root)
      return {};
    if (root->CanFetchMore())
      root->FetchMore();
    env_.executor_.Poll();

    std::vector<std::u16string> rows;
    for (void* node : view_->tree_view().GetChildNodes(root)) {
      auto* tree_node = static_cast<ConfigurationTreeNode*>(node);
      if (tree_node->IsLoadingPlaceholder())
        continue;
      rows.push_back(view_->model().GetText(tree_node, /*column_id=*/0));
    }
    return rows;
  }

  // Ascending ids, so a comparator consulting them puts Яблоко first.
  static inline const scada::NodeId kFirstTypeId{4001, 1};
  static inline const scada::NodeId kSecondTypeId{4002, 1};
  static inline const scada::NodeId kThirdTypeId{4003, 1};

  AppEnvironment app_env_;
  ControllerEnvironment env_;
  StaticNodeService node_service_;
  std::unique_ptr<TestConfigurationTreeView> view_;
  std::unique_ptr<UiView> ui_view_;
};

TEST_F(ConfigurationTreeViewSortTest, ObjectsReadInNameOrderNotTypeIdOrder) {
  // Before the fix: Яблоко, Берёза, Астра, Аметист — the three objects in
  // ascending type-definition id, which is the reverse of what a reader wants
  // and corresponds to nothing on screen.
  EXPECT_THAT(VisibleRows(),
              ElementsAre(u"Астра", u"Берёза", u"Яблоко", u"Аметист"));
}
