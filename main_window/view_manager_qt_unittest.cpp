#include "aui/dialog_service_mock.h"
#include "aui/test/app_environment.h"
#include "base/check.h"
#include "base/test/test_executor.h"
#include "controller/controller_factory_mock.h"
#include "controller/controller_mock.h"
#include "controller/controller_registry.h"
#include "controller/window_info.h"
#include "main_window/view_manager.h"
#include "main_window/view_manager_delegate_mock.h"
#include "modules/web/web_component.h"
#include "view_manager_qt_component.h"

#include <QMainWindow>
#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

struct ViewState {
  ViewState(std::string_view window_type, ViewManager& view_manager)
      : view_manager_{view_manager}, window_definition{window_type} {
    auto widget = std::make_unique<QWidget>();

    QObject::connect(widget.get(), &QObject::destroyed,
                     widget_destroyed.AsStdFunction());

    EXPECT_CALL(*controller, Init(_))
        .WillOnce(Return(ByMove(std::move(widget))));
  }

  ~ViewState() {
    EXPECT_CALL(widget_destroyed, Call());

    view_manager_.CloseView(*opened_view);
  }

  ViewManager& view_manager_;

  WindowDefinition window_definition;

  NiceMock<MockFunction<void()>> widget_destroyed;

  std::unique_ptr<StrictMock<MockController>> controller =
      std::make_unique<StrictMock<MockController>>();

  OpenedView* opened_view = nullptr;
};

// A WIN_SINGLE_ITEM view: one bound to a single node, which is what makes
// opening it again for the same node a request for the window that is already
// there. The command id is a test-only sentinel so it cannot collide with a
// statically registered controller.
const WindowInfo kSingleItemWindowInfo = {0x7fff0001, "SingleItemTest",
                                          u"Single Item", WIN_SINGLE_ITEM};

class ViewManagerTest : public Test {
 public:
  virtual void SetUp() override;
  virtual void TearDown() override;

 protected:
  std::unique_ptr<ViewState> ExpectOpenView(
      const WindowInfo& window_info = kWindowInfo);

  AppEnvironment app_env_;

  TestExecutor executor_;

  ControllerRegistry controller_registry_;

  QMainWindow main_window_;

  StrictMock<MockDialogService> dialog_service_;
  StrictMock<MockViewManagerDelegate> view_manager_delegate_;
  StrictMock<MockControllerFactory> controller_factory_;

  ViewManager view_manager_qt_{main_window_, view_manager_delegate_};

  inline static const WindowInfo& kWindowInfo = kWebWindowInfo;
};

void ViewManagerTest::SetUp() {
  const auto never_called = [](const ControllerContext&) {
    // This handler is intercepted by fake `controller_factory_` and
    // must never invoked.
    scada::base::NotReached();
    return nullptr;
  };
  controller_registry_.AddControllerFactory(kWindowInfo, never_called);
  controller_registry_.AddControllerFactory(kSingleItemWindowInfo,
                                            never_called);

  // Opening, activating, and closing views drives active-view-changed
  // notifications (see ViewManager). These tests assert on view creation and
  // teardown, not active-view tracking, so tolerate the notifications.
  EXPECT_CALL(view_manager_delegate_, OnActiveViewChanged(_))
      .Times(AnyNumber());
}

void ViewManagerTest::TearDown() {}

std::unique_ptr<ViewState> ViewManagerTest::ExpectOpenView(
    const WindowInfo& window_info) {
  auto view_state =
      std::make_unique<ViewState>(window_info.name, view_manager_qt_);

  auto new_opened_view = std::make_unique<OpenedView>(OpenedViewContext{
      .executor_ = executor_,
      .window_info_ = window_info,
      .window_def_ = view_state->window_definition,
      .dialog_service_ = dialog_service_,
      .controller_factory_ = controller_factory_.AsStdFunction()});

  EXPECT_CALL(controller_factory_, Call(window_info.command_id, _, _))
      .WillOnce(Return(ByMove(std::move(view_state->controller))));

  new_opened_view->Init();

  view_state->opened_view = new_opened_view.get();

  // The call gets a reference  to another `WindowDefinition`.
  EXPECT_CALL(view_manager_delegate_, OnCreateView(/*window_def=*/_))
      .WillOnce(Return(ByMove(std::move(new_opened_view))));

  EXPECT_CALL(view_manager_delegate_,
              OnViewClosed(Ref(*view_state->opened_view)));

  return view_state;
}

TEST_F(ViewManagerTest, CloseView_DeletesNativeView) {
  auto view_state = ExpectOpenView();
  view_manager_qt_.OpenView(view_state->window_definition, /*make_active=*/true,
                            /*after_view=*/nullptr);
}

TEST_F(ViewManagerTest, SplitAndClose) {
  auto view_state1 = ExpectOpenView();
  view_manager_qt_.OpenView(view_state1->window_definition,
                            /*make_active=*/true, /*after_view=*/nullptr);

  auto view_state2 = ExpectOpenView();
  auto* opened_view2 =
      view_manager_qt_.OpenView(view_state2->window_definition,
                                /*make_active=*/true, /*after_view=*/nullptr);

  view_manager_qt_.SplitView(*opened_view2, /*vertically=*/false);
}

// The regression: OpenView deduplicated panes by type and nothing else, so a
// non-pane view opened again for the same node produced a second identical
// tab. Properties (`NewProps`) is the WIN_SINGLE_ITEM view this bit.
TEST_F(ViewManagerTest, SingleItemView_OpeningTheSameNodeTwiceReusesTheView) {
  auto view_state = ExpectOpenView(kSingleItemWindowInfo);
  view_state->window_definition.AddItem("Item").SetString("path", "node-1");

  auto* first = view_manager_qt_.OpenView(view_state->window_definition,
                                          /*make_active=*/true,
                                          /*after_view=*/nullptr);
  ASSERT_TRUE(first);

  // A second OnCreateView would fail the strict delegate mock, which is half
  // the assertion: no view is built at all the second time.
  auto* second = view_manager_qt_.OpenView(view_state->window_definition,
                                           /*make_active=*/true,
                                           /*after_view=*/nullptr);

  EXPECT_EQ(second, first);
}

// The node is part of a single-item view's identity, so two nodes still get
// two windows.
TEST_F(ViewManagerTest, SingleItemView_ADifferentNodeGetsItsOwnView) {
  auto view_state1 = ExpectOpenView(kSingleItemWindowInfo);
  view_state1->window_definition.AddItem("Item").SetString("path", "node-1");
  auto* first = view_manager_qt_.OpenView(view_state1->window_definition,
                                          /*make_active=*/true,
                                          /*after_view=*/nullptr);

  auto view_state2 = ExpectOpenView(kSingleItemWindowInfo);
  view_state2->window_definition.AddItem("Item").SetString("path", "node-2");
  auto* second = view_manager_qt_.OpenView(view_state2->window_definition,
                                           /*make_active=*/true,
                                           /*after_view=*/nullptr);

  EXPECT_NE(second, first);
  EXPECT_TRUE(second);
}

// A view without WIN_SINGLE_ITEM keeps the old behaviour: it is not bound to
// one node, so a second open is a second window on purpose.
TEST_F(ViewManagerTest, PlainView_OpeningTwiceStillOpensTwoViews) {
  auto view_state1 = ExpectOpenView();
  auto* first = view_manager_qt_.OpenView(view_state1->window_definition,
                                          /*make_active=*/true,
                                          /*after_view=*/nullptr);

  auto view_state2 = ExpectOpenView();
  auto* second = view_manager_qt_.OpenView(view_state2->window_definition,
                                           /*make_active=*/true,
                                           /*after_view=*/nullptr);

  EXPECT_NE(second, first);
}

TEST(ViewManagerQtComponentTest, AddSplitSaveAndRemovePlainWidgets) {
  AppEnvironment app_env;
  QMainWindow main_window;
  ViewManagerQtComponent component{main_window};

  auto widget1 = std::make_unique<QWidget>();
  auto widget2 = std::make_unique<QWidget>();

  std::vector<ViewManagerQtComponent::ViewInfo> views{
      {.id = 1, .widget = widget1.get(), .title = u"One"},
      {.id = 2, .widget = widget2.get(), .title = u"Two"}};

  component.AddView(views[0], std::nullopt);
  component.AddView(views[1], views[0].id);
  component.SplitView(views[1].id, /*vertically=*/false);

  auto layout = component.SaveLayout(views);
  EXPECT_EQ(layout.main.type, ViewManagerQtComponent::LayoutNode::Type::Split);
  EXPECT_TRUE(layout.main.split_vertical);
  ASSERT_TRUE(layout.main.left);
  ASSERT_TRUE(layout.main.right);
  EXPECT_THAT(layout.main.left->tabs, ElementsAre(views[0].id));
  EXPECT_THAT(layout.main.right->tabs, ElementsAre(views[1].id));

  ASSERT_TRUE(component.RemoveView(views[0].id));
  ASSERT_TRUE(component.RemoveView(views[1].id));
  widget1->setParent(nullptr);
  widget2->setParent(nullptr);
}
