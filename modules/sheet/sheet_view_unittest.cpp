#include "modules/sheet/sheet_view.h"

#include "aui/grid.h"
#include "aui/test/app_environment.h"
#include "common/node_state.h"
#include "controller/selection_model.h"
#include "controller/test/controller_environment.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "profile/window_definition.h"
#include "timed_data/timed_data_fake.h"

#include <QItemSelectionModel>
#include <QTableView>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace {

constexpr scada::NodeId kItemA{101, 1};
constexpr scada::NodeId kItemB{102, 1};

class NodeFakeTimedData final : public FakeTimedData {
 public:
  explicit NodeFakeTimedData(NodeRef node) : node_{std::move(node)} {}

  NodeRef GetNode() const override { return node_; }

 private:
  NodeRef node_;
};

class SheetControllerTest : public Test {
 protected:
  SheetControllerTest() {
    for (const scada::NodeId& id : {kItemA, kItemB}) {
      node_service_.Add(
          scada::NodeState{.node_id = id,
                           .node_class = scada::NodeClass::Variable,
                           .type_definition_id = scada::id::BaseVariableType});
    }
    // Every formula resolves to the node it names, the way the real service
    // binds a cell that holds a bare node id.
    ON_CALL(env_.timed_data_service_, GetFormulaTimedData(_, _))
        .WillByDefault([this](std::string_view formula,
                              const scada::AggregateFilter&) {
          const scada::NodeId id =
              formula.find("101") != std::string_view::npos ? kItemA : kItemB;
          return std::make_shared<NodeFakeTimedData>(node_service_.GetNode(id));
        });
  }

  AppEnvironment app_env_;
  ControllerEnvironment env_;
  StaticNodeService node_service_;
};

}  // namespace

// «Сводка»/«График» on a cell range collect the range's nodes through this
// list, which kept only cells with NO node — an inverted test — so a range
// opened nothing (backlog 927).
TEST_F(SheetControllerTest, ARangeSelectsTheNodesOfItsCells) {
  WindowDefinition definition{"CusTable"};
  definition.AddItem("SheetCell")
      .SetInt("row", 1)
      .SetInt("col", 1)
      .SetString("text", "=" + NodeIdToScadaString(kItemA));
  definition.AddItem("SheetCell")
      .SetInt("row", 2)
      .SetInt("col", 1)
      .SetString("text", "=" + NodeIdToScadaString(kItemB));

  SheetController controller{env_.MakeControllerContext()};
  std::unique_ptr<UiView> view = controller.Init(definition);
  // Grid has no Q_OBJECT, so it is found as the QTableView it is.
  auto* grid = view->findChild<QTableView*>();
  ASSERT_NE(grid, nullptr);

  grid->selectionModel()->select(
      QItemSelection{grid->model()->index(0, 0), grid->model()->index(2, 2)},
      QItemSelectionModel::ClearAndSelect);

  EXPECT_THAT(controller.GetSelectionModel()->GetMultipleNodeIds(),
              UnorderedElementsAre(kItemA, kItemB));
}
