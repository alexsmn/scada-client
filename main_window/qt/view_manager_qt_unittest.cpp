#include "main_window/qt/view_manager_qt.h"

#include "aui/dialog_service_mock.h"
#include "aui/test/app_environment.h"
#include "base/test/test_executor.h"
#include "components/web/web_component.h"
#include "controller/controller_factory_mock.h"
#include "controller/controller_mock.h"
#include "controller/window_info.h"
#include "main_window/view_manager_delegate_mock.h"

#include <QMainWindow>
#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

struct ViewState {
  explicit ViewState(ViewManager& view_manager) : view_manager_{view_manager} {
    auto* widget = new QWidget;

    QObject::connect(widget, &QObject::destroyed,
                     widget_destroyed.AsStdFunction());

    EXPECT_CALL(*controller, Init(_)).WillOnce(Return(widget));
  }

  ~ViewState() {
    EXPECT_CALL(widget_destroyed, Call());

    view_manager_.CloseView(*opened_view);
  }

  ViewManager& view_manager_;

  WindowDefinition window_definition{kWindowInfo};

  NiceMock<MockFunction<void()>> widget_destroyed;

  std::unique_ptr<StrictMock<MockController>> controller =
      std::make_unique<StrictMock<MockController>>();

  OpenedView* opened_view = nullptr;

  inline static const WindowInfo& kWindowInfo = kWebWindowInfo;
};

class ViewManagerQtTest : public Test {
 public:
  virtual void TearDown() override;

 protected:
  std::unique_ptr<ViewState> ExpectOpenView();

  AppEnvironment app_env_;

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  QMainWindow main_window_;

  StrictMock<MockDialogService> dialog_service_;
  StrictMock<MockViewManagerDelegate> view_manager_delegate_;
  StrictMock<MockControllerFactory> controller_factory_;

  ViewManagerQt view_manager_qt_{main_window_, view_manager_delegate_};
};

void ViewManagerQtTest::TearDown() {}

std::unique_ptr<ViewState> ViewManagerQtTest::ExpectOpenView() {
  auto view_state = std::make_unique<ViewState>(view_manager_qt_);

  auto new_opened_view = std::make_unique<OpenedView>(OpenedViewContext{
      .executor_ = executor_,
      .window_info_ = ViewState::kWindowInfo,
      .window_def_ = view_state->window_definition,
      .dialog_service_ = dialog_service_,
      .controller_factory_ = controller_factory_.AsStdFunction()});

  EXPECT_CALL(controller_factory_,
              Call(ViewState::kWindowInfo.command_id, _, _))
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

TEST_F(ViewManagerQtTest, CloseView_DeletesNativeView) {
  auto view_state = ExpectOpenView();
  view_manager_qt_.OpenView(view_state->window_definition, /*make_active=*/true,
                            /*after_view=*/nullptr);
}

TEST_F(ViewManagerQtTest, SplitAndClose) {
  auto view_state1 = ExpectOpenView();
  auto* opened_view1 =
      view_manager_qt_.OpenView(view_state1->window_definition,
                                /*make_active=*/true, /*after_view=*/nullptr);
  opened_view1;

  auto view_state2 = ExpectOpenView();
  auto* opened_view2 =
      view_manager_qt_.OpenView(view_state2->window_definition,
                                /*make_active=*/true, /*after_view=*/nullptr);

  view_manager_qt_.SplitView(*opened_view2, /*vertically=*/false);
}
