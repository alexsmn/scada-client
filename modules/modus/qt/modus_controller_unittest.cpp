#include "modus/qt/modus_controller.h"

#include "aui/test/app_environment.h"
#include "common/vds_runtime_api.h"
#include "controller/test/controller_environment.h"
#include "modus/modus_view_wrapper.h"
#include "profile/window_definition.h"

#include <QPointer>
#include <QWidget>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace {

using testing::IsNull;
using testing::NotNull;

// A `ModusViewWrapper` with no VDS runtime behind it. The production wrapper is
// a `VdsRuntimeWidget`, which dlopens `tc_vds_runtime` — a shared library built
// by the designer product, not by `client/` (ADR 0011) — so a test that used it
// would be testing whether that library happened to be installed. This records
// what the controller asked of the wrapper instead.
class FakeModusViewWrapper final : public ModusViewWrapper {
 public:
  void Open(const WindowDefinition& definition,
            int32_t document_kind) override {
    opened_paths_.push_back(definition.path);
    opened_kinds_.push_back(document_kind);
  }

  void Save(WindowDefinition& definition) override { ++save_count_; }

  std::filesystem::path GetPath() const override { return path_; }

  bool ShowContainedItem(const scada::NodeId& item_id) override {
    shown_items_.push_back(item_id);
    return show_contained_item_result_;
  }

  void set_path(std::filesystem::path path) { path_ = std::move(path); }
  void set_show_contained_item_result(bool result) {
    show_contained_item_result_ = result;
  }

  const std::vector<std::filesystem::path>& opened_paths() const {
    return opened_paths_;
  }
  const std::vector<int32_t>& opened_kinds() const { return opened_kinds_; }
  int save_count() const { return save_count_; }
  const std::vector<scada::NodeId>& shown_items() const { return shown_items_; }

 private:
  std::filesystem::path path_;
  bool show_contained_item_result_ = false;
  std::vector<std::filesystem::path> opened_paths_;
  std::vector<int32_t> opened_kinds_;
  int save_count_ = 0;
  std::vector<scada::NodeId> shown_items_;
};

class ModusControllerTest : public testing::Test {
 protected:
  // The widget the controller embeds. Owned by the `UiView` that `Init`
  // returns, exactly as the production `QScrollArea` is.
  ModusController MakeController() {
    return ModusController{
        controller_env_.MakeControllerContext(),
        [this](ModusController&) -> ModusController::RuntimeView {
          return {.widget = new QWidget, .wrapper = &wrapper_};
        }};
  }

  // Declared before `controller_env_`: the environment's mocks and the widgets
  // the tests build both need a live QApplication, and members die in reverse
  // declaration order.
  AppEnvironment app_env_;
  ControllerEnvironment controller_env_;
  FakeModusViewWrapper wrapper_;
};

TEST_F(ModusControllerTest, InitOpensTheDocumentNamedByTheWindowDefinition) {
  ModusController controller = MakeController();

  WindowDefinition definition;
  definition.path = "schemes/substation.sde";

  std::unique_ptr<UiView> view = controller.Init(definition);

  EXPECT_THAT(view, NotNull());
  EXPECT_THAT(
      wrapper_.opened_paths(),
      testing::ElementsAre(std::filesystem::path{"schemes/substation.sde"}));
}

// Regression guard for the ownership shape: `Init` hands the widget to the
// caller, so the controller must not also delete it. A double-owned widget
// crashes on window close rather than in the test that created it.
TEST_F(ModusControllerTest, InitTransfersWidgetOwnershipToTheCaller) {
  ModusController controller = MakeController();

  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});
  ASSERT_THAT(view, NotNull());

  QPointer<QWidget> observer{view.get()};
  view.reset();

  EXPECT_TRUE(observer.isNull());
}

TEST_F(ModusControllerTest, SaveWritesTheWrappersPathBackIntoTheDefinition) {
  ModusController controller = MakeController();
  controller.Init(WindowDefinition{});

  wrapper_.set_path("schemes/renamed.sde");

  WindowDefinition definition;
  controller.Save(definition);

  EXPECT_EQ(definition.path.filename(), std::filesystem::path{"renamed.sde"});
  EXPECT_EQ(wrapper_.save_count(), 1);
}

TEST_F(ModusControllerTest, ShowContainedItemDelegatesToTheWrapper) {
  ModusController controller = MakeController();
  controller.Init(WindowDefinition{});

  const scada::NodeId item_id{42, 1};

  wrapper_.set_show_contained_item_result(false);
  EXPECT_FALSE(controller.ShowContainedItem(item_id));

  wrapper_.set_show_contained_item_result(true);
  EXPECT_TRUE(controller.ShowContainedItem(item_id));

  EXPECT_THAT(wrapper_.shown_items(), testing::ElementsAre(item_id, item_id));
}

// The data binding: clicking a drawing element whose data source names a node
// selects that node's live value, which is what drives the Inspector and every
// selection-scoped command.
TEST_F(ModusControllerTest, SelectingADataSourceSelectsThatNodesLiveValue) {
  ModusController controller = MakeController();
  controller.Init(WindowDefinition{});

  SelectionModel* selection = controller.GetSelectionModel();
  ASSERT_THAT(selection, NotNull());
  ASSERT_TRUE(selection->empty());

  controller.SelectDataSource("ns=1;i=42");

  EXPECT_FALSE(selection->empty());
}

// A click that hits nothing reports an empty data source, and must not disturb
// a selection the operator already made elsewhere.
TEST_F(ModusControllerTest, AnEmptyDataSourceLeavesTheSelectionAlone) {
  ModusController controller = MakeController();
  controller.Init(WindowDefinition{});

  SelectionModel* selection = controller.GetSelectionModel();
  controller.SelectDataSource("ns=1;i=42");
  ASSERT_FALSE(selection->empty());

  controller.SelectDataSource("");

  EXPECT_FALSE(selection->empty());
}

// Pins the surprising half of `scada::NodeId::FromString`: it has no failure
// mode. An unparseable data source becomes a *string* NodeId rather than
// clearing the selection, so a drawing naming a stale tag selects a node that
// does not resolve. Documented here because the controller's catch reads as
// though it guarded against this, and it does not.
TEST_F(ModusControllerTest, AnUnparseableDataSourceStillSelectsAStringNode) {
  ModusController controller = MakeController();
  controller.Init(WindowDefinition{});

  SelectionModel* selection = controller.GetSelectionModel();

  controller.SelectDataSource("not a node id");

  EXPECT_FALSE(selection->empty());
}

TEST_F(ModusControllerTest, AcknowledgeSelectionIsSafeWithNothingSelected) {
  ModusController controller = MakeController();
  controller.Init(WindowDefinition{});

  ASSERT_TRUE(controller.GetSelectionModel()->empty());

  controller.AcknowledgeSelection();

  EXPECT_TRUE(controller.GetSelectionModel()->empty());
}

// The controller registers no commands of its own today. Pinned so that adding
// one is a deliberate change to this expectation rather than a silent shift —
// task 140 ("implement Modus commands") lands exactly here.
TEST_F(ModusControllerTest, NoCommandsAreRegisteredYet) {
  ModusController controller = MakeController();
  controller.Init(WindowDefinition{});

  EXPECT_THAT(controller.GetCommandHandler(0), IsNull());
}

// Task 483: the «Use Modus runtime renderer» command used to toggle a profile
// flag that nothing read, so it changed no rendering. `Init` now derives the
// document kind from the definition and the profile and passes it to the
// runtime, which is what makes the operator's choice reach the renderer.
TEST_F(ModusControllerTest,
       InitOpensAnXsdeWithTheVersionTwoKindWhenTheFlagIsSet) {
  controller_env_.profile_.modus.modus2 = true;
  ModusController controller = MakeController();

  WindowDefinition definition;
  definition.path = "schemes/substation.xsde";

  std::unique_ptr<UiView> view = controller.Init(definition);
  ASSERT_THAT(view, NotNull());

  EXPECT_THAT(wrapper_.opened_kinds(),
              testing::ElementsAre(TC_VDS_RUNTIME_DOCUMENT_KIND_XSDE));
}

TEST_F(ModusControllerTest,
       InitOpensAnXsdeWithTheVersionOneKindWhenTheFlagIsClear) {
  controller_env_.profile_.modus.modus2 = false;
  ModusController controller = MakeController();

  WindowDefinition definition;
  definition.path = "schemes/substation.xsde";

  std::unique_ptr<UiView> view = controller.Init(definition);
  ASSERT_THAT(view, NotNull());

  EXPECT_THAT(wrapper_.opened_kinds(),
              testing::ElementsAre(TC_VDS_RUNTIME_DOCUMENT_KIND_SDE));
}

// An `.sde` is version 1 whatever the profile says, so the flag must not reach
// it. This is the case that would regress if `DocumentKindFor` were reduced to
// reading the profile alone.
TEST_F(ModusControllerTest,
       InitOpensAnSdeWithTheVersionOneKindEvenWithTheFlagSet) {
  controller_env_.profile_.modus.modus2 = true;
  ModusController controller = MakeController();

  WindowDefinition definition;
  definition.path = "schemes/substation.sde";

  std::unique_ptr<UiView> view = controller.Init(definition);
  ASSERT_THAT(view, NotNull());

  EXPECT_THAT(wrapper_.opened_kinds(),
              testing::ElementsAre(TC_VDS_RUNTIME_DOCUMENT_KIND_SDE));
}

}  // namespace
