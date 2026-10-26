#include "modus/qt/modus_controller.h"

#include "aui/test/app_environment.h"
#include "controller/test/controller_environment.h"
#include "filesystem/file_cache.h"
#include "modus/modus_view_wrapper.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "scada/attribute_service.h"
#include "timed_data/timed_data_mock.h"

#include <QPointer>
#include <QWidget>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

using testing::_;
using testing::Eq;
using testing::IsNull;
using testing::NotNull;
using testing::Return;

// A `ModusViewWrapper` with no display runtime behind it. The production
// wrapper is a `DisplayWidget`, which loads `display_runtime` — a shared
// library built by the display product, not by `client/` (ADR 0013) — so a
// test that used it would be testing whether that library happened to be
// installed. This records
// what the controller asked of the wrapper instead.
class FakeModusViewWrapper final : public ModusViewWrapper {
 public:
  void Open(const WindowDefinition& definition) override {
    opened_paths_.push_back(definition.path);
  }

  void Save(WindowDefinition& definition) override { ++save_count_; }

  std::filesystem::path GetPath() const override { return path_; }

  bool ShowContainedItem(const scada::NodeId& item_id) override {
    shown_items_.push_back(item_id);
    return show_contained_item_result_;
  }

  std::u16string GetTitle() const override { return title_; }
  void set_title(std::u16string title) { title_ = std::move(title); }

  std::vector<std::string> GetDataSources() const override {
    return data_sources_;
  }

  void SetDataSourceReading(const ModusDataSourceReading& reading) override {
    readings_.push_back(reading);
  }

  bool SelectDataSourceElement(std::string_view data_source) override {
    selected_elements_.emplace_back(data_source);
    return true;
  }

  void set_data_sources(std::vector<std::string> data_sources) {
    data_sources_ = std::move(data_sources);
  }
  const std::vector<ModusDataSourceReading>& readings() const {
    return readings_;
  }
  const std::vector<std::string>& selected_elements() const {
    return selected_elements_;
  }

  void set_path(std::filesystem::path path) { path_ = std::move(path); }
  void set_show_contained_item_result(bool result) {
    show_contained_item_result_ = result;
  }

  const std::vector<std::filesystem::path>& opened_paths() const {
    return opened_paths_;
  }
  int save_count() const { return save_count_; }
  const std::vector<scada::NodeId>& shown_items() const { return shown_items_; }

 private:
  std::filesystem::path path_;
  bool show_contained_item_result_ = false;
  std::vector<std::filesystem::path> opened_paths_;
  int save_count_ = 0;
  std::vector<scada::NodeId> shown_items_;
  std::u16string title_;
  std::vector<std::string> data_sources_;
  std::vector<ModusDataSourceReading> readings_;
  std::vector<std::string> selected_elements_;
};

// The latest reading the wrapper was given for `data_source`.
std::optional<ModusDataSourceReading> LastReading(
    const FakeModusViewWrapper& wrapper,
    std::string_view data_source) {
  std::optional<ModusDataSourceReading> last;
  for (const ModusDataSourceReading& reading : wrapper.readings()) {
    if (reading.data_source == data_source)
      last = reading;
  }
  return last;
}

// OPC UA Bad severity, as the readings carry it.
constexpr int32_t kBadQuality = static_cast<int32_t>(0x80000000u);

class ModusControllerTest : public testing::Test {
 protected:
  // The widget the controller embeds. Owned by the `UiView` that `Init`
  // returns, exactly as the production `QScrollArea` is.
  ModusController MakeController() {
    return ModusController{
        controller_env_.MakeControllerContext(),
        [this](ModusController&) -> ModusController::DisplayView {
          return {.widget = new QWidget, .wrapper = &wrapper_};
        }};
  }

  // Gives `formula` its own live value, as the formula service would.
  std::shared_ptr<MockTimedData> BindFormula(const std::string& formula) {
    auto data = std::make_shared<testing::NiceMock<MockTimedData>>();
    ON_CALL(controller_env_.timed_data_service_,
            GetFormulaTimedData(Eq(formula), _))
        .WillByDefault(Return(data));
    return data;
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

// A click on bare page or on an unbound shape reports an empty data source.
// The display moves its highlight off the previous object, so the selection
// must clear too — otherwise commands act on an object no longer highlighted
// (backlog 907).
TEST_F(ModusControllerTest, AnEmptyDataSourceClearsTheSelection) {
  ModusController controller = MakeController();
  controller.Init(WindowDefinition{});

  SelectionModel* selection = controller.GetSelectionModel();
  controller.SelectDataSource("ns=1;i=42");
  ASSERT_FALSE(selection->empty());

  controller.SelectDataSource("");

  EXPECT_TRUE(selection->empty());
}

// Pins the surprising half of the formula service: it has no failure mode
// that throws. A data source it cannot resolve yields an error TimedData
// rather than clearing the selection, so a drawing naming a stale alias
// selects an object that shows as not found. Documented here because the
// controller's catch reads as though it guarded against this, and it does
// not.
TEST_F(ModusControllerTest, AnUnresolvableDataSourceStillSelectsSomething) {
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

// Backlog 491 retired the «Use Modus runtime renderer» flag and the
// `DocumentKind` it produced: the reader chooses the SDE or XSDE decoder from
// the extension, which is the only thing that can, so three cases asserting
// the kind went with it. What is left to pin is that `Init` opens the
// definition's own document, for both extensions.
TEST_F(ModusControllerTest, InitOpensTheDefinitionsDocument) {
  ModusController controller = MakeController();

  WindowDefinition definition;
  definition.path = "schemes/substation.xsde";

  std::unique_ptr<UiView> view = controller.Init(definition);
  ASSERT_THAT(view, NotNull());

  EXPECT_THAT(wrapper_.opened_paths(), testing::ElementsAre(definition.path));
}

TEST_F(ModusControllerTest, InitOpensAnSdeDocumentToo) {
  ModusController controller = MakeController();

  WindowDefinition definition;
  definition.path = "schemes/substation.sde";

  std::unique_ptr<UiView> view = controller.Init(definition);
  ASSERT_THAT(view, NotNull());

  EXPECT_THAT(wrapper_.opened_paths(), testing::ElementsAre(definition.path));
}

// ── Live values (backlog 915) ───────────────────────────────────────────────

// The 915 defect: nothing in the Client subscribed to a schematic's bindings,
// so a value change never reached the drawing. Each binding key the document
// enumerates is now subscribed through the formula service, and a change is
// delivered for that data source and no other.
TEST_F(ModusControllerTest, AValueChangeReachesTheElementsBoundToIt) {
  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  std::shared_ptr<MockTimedData> voltage = BindFormula("TIT_U");
  breaker->data_value_ = scada::MakeReadResult(true);
  voltage->data_value_ = scada::MakeReadResult(10.5);
  wrapper_.set_data_sources({"TS_Q1", "TIT_U"});

  ModusController controller = MakeController();
  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});

  // Both are drawn from the value they have on connecting.
  ASSERT_TRUE(LastReading(wrapper_, "TS_Q1"));
  EXPECT_EQ(LastReading(wrapper_, "TS_Q1")->state, "closed");
  EXPECT_EQ(LastReading(wrapper_, "TS_Q1")->quality, 0);
  ASSERT_TRUE(LastReading(wrapper_, "TIT_U"));
  EXPECT_FALSE(LastReading(wrapper_, "TIT_U")->text.empty());

  const size_t voltage_readings = std::ranges::count_if(
      wrapper_.readings(),
      [](const ModusDataSourceReading& r) { return r.data_source == "TIT_U"; });

  breaker->UpdateData(scada::MakeReadResult(false));

  EXPECT_EQ(LastReading(wrapper_, "TS_Q1")->state, "open");
  EXPECT_EQ(std::ranges::count_if(wrapper_.readings(),
                                  [](const ModusDataSourceReading& r) {
                                    return r.data_source == "TIT_U";
                                  }),
            voltage_readings);
}

TEST_F(ModusControllerTest, ABadReadingIsReportedAsBad) {
  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  wrapper_.set_data_sources({"TS_Q1"});

  ModusController controller = MakeController();
  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});

  scada::DataValue offline = scada::MakeReadResult(true);
  offline.qualifier = scada::Qualifier{scada::Qualifier::OFFLINE};
  breaker->UpdateData(std::move(offline));

  EXPECT_EQ(LastReading(wrapper_, "TS_Q1")->quality, kBadQuality);
}

// A key the formula service has nothing for is drawn as not connected, and
// does not stop the rest of the schematic binding.
TEST_F(ModusControllerTest, AnUnresolvedBindingIsReportedAsBad) {
  ON_CALL(controller_env_.timed_data_service_,
          GetFormulaTimedData(Eq("MISSING"), _))
      .WillByDefault(Return(nullptr));
  BindFormula("TS_Q1")->data_value_ = scada::MakeReadResult(true);
  wrapper_.set_data_sources({"MISSING", "TS_Q1"});

  ModusController controller = MakeController();
  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});

  ASSERT_TRUE(LastReading(wrapper_, "MISSING"));
  EXPECT_EQ(LastReading(wrapper_, "MISSING")->quality, kBadQuality);
  EXPECT_EQ(LastReading(wrapper_, "MISSING")->state, "");
  EXPECT_EQ(LastReading(wrapper_, "TS_Q1")->state, "closed");
}

// Once the window has destroyed the view, a late value change must not reach
// the wrapper that lived in it.
TEST_F(ModusControllerTest, NoReadingIsDeliveredAfterTheViewIsGone) {
  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  wrapper_.set_data_sources({"TS_Q1"});

  ModusController controller = MakeController();
  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});
  const size_t readings = wrapper_.readings().size();

  view.reset();
  breaker->UpdateData(scada::MakeReadResult(false));

  EXPECT_EQ(wrapper_.readings().size(), readings);
}

// A click selects the element's object through the formula service, so an
// alias selects what it names rather than a string node nobody has.
TEST_F(ModusControllerTest, SelectingAnAliasSelectsTheObjectItNames) {
  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  EXPECT_CALL(controller_env_.timed_data_service_,
              GetFormulaTimedData(Eq("TS_Q1"), _))
      .Times(testing::AtLeast(1));

  ModusController controller = MakeController();
  controller.Init(WindowDefinition{});

  controller.SelectDataSource("TS_Q1");

  ASSERT_FALSE(controller.GetSelectionModel()->empty());
  EXPECT_EQ(controller.GetSelectionModel()->timed_data(),
            TimedDataSpec{breaker});
}

// Backlog 910: «Мнемосхема» for an object selects the element bound to it.
TEST_F(ModusControllerTest, ShowContainedItemSelectsTheElementBoundToTheNode) {
  const scada::NodeId breaker_node{42, 1};
  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  ON_CALL(*breaker, GetNode())
      .WillByDefault(
          Return(NodeRef{breaker_node, &controller_env_.node_service_}));
  BindFormula("TIT_U");
  wrapper_.set_data_sources({"TIT_U", "TS_Q1"});

  ModusController controller = MakeController();
  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});

  EXPECT_TRUE(controller.ShowContainedItem(breaker_node));
  EXPECT_THAT(wrapper_.selected_elements(), testing::ElementsAre("TS_Q1"));
  EXPECT_TRUE(wrapper_.shown_items().empty());

  // A node no element is bound to falls back to the renderer.
  EXPECT_FALSE(controller.ShowContainedItem(scada::NodeId{43, 1}));
  EXPECT_THAT(wrapper_.shown_items(),
              testing::ElementsAre(scada::NodeId{43, 1}));
}

// The display menu and «Мнемосхема» both read the file cache, which only the
// unbuilt ActiveX loader ever wrote. Opening a schematic now indexes it under
// its «Заголовок» and the objects it is bound to (backlogs 915, 910).
TEST_F(ModusControllerTest, OpeningASchematicIndexesItsTitleAndObjects) {
  const scada::NodeId breaker_node{42, 1};
  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  ON_CALL(*breaker, GetNode())
      .WillByDefault(
          Return(NodeRef{breaker_node, &controller_env_.node_service_}));
  wrapper_.set_data_sources({"TS_Q1"});
  wrapper_.set_path("schemes/substation.sde");
  wrapper_.set_title(u"Substation 1");

  ModusController controller = MakeController();
  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});

  const FileCache::FileList& list =
      controller_env_.file_cache_.GetList(ID_MODUS_VIEW);
  const int index = list.Find("substation.sde");
  ASSERT_NE(index, -1);
  EXPECT_EQ(list[index].title, u"Substation 1");
  EXPECT_TRUE(list[index].items.contains(breaker_node));
  EXPECT_THAT(list.GetFilesContainingItem(breaker_node), testing::SizeIs(1));
}

TEST_F(ModusControllerTest, AnUntitledSchematicKeepsItsFileName) {
  wrapper_.set_path("schemes/substation.sde");

  ModusController controller = MakeController();
  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});

  const FileCache::FileList& list =
      controller_env_.file_cache_.GetList(ID_MODUS_VIEW);
  const int index = list.Find("substation.sde");
  ASSERT_NE(index, -1);
  EXPECT_EQ(list[index].title, u"substation");
}

}  // namespace
