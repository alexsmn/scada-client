#include "components/main/qt/view_manager_qt.h"

#include "base/test/test_executor.h"
#include "components/main/view_manager_delegate_mock.h"
#include "controller_mock.h"
#include "services/dialog_service_mock.h"
#include "window_info.h"

#include <QApplication>
#include <QMainWindow>
#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

class ViewManagerQtTest : public Test {
 public:
  ViewManagerQtTest();
  ~ViewManagerQtTest();

 protected:
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  int argc_ = 0;
  char* argv_ = nullptr;
  QApplication application_{argc_, &argv_};

  QMainWindow main_window_;

  StrictMock<MockDialogService> dialog_service_;
  StrictMock<MockViewManagerDelegate> view_manager_delegate_;

  ViewManagerQt view_manager_qt_{main_window_, view_manager_delegate_};
};

ViewManagerQtTest::ViewManagerQtTest() {}

ViewManagerQtTest::~ViewManagerQtTest() {}

TEST_F(ViewManagerQtTest, CloseView_DeletesNativeView) {
  const WindowInfo window_info{123, "name", L"title"};
  WindowDefinition window_definition{window_info};

  auto controller = std::make_unique<StrictMock<MockController>>();
  auto& controller_ref = *controller;

  ControllerFactory controller_factory =
      [&](unsigned command_id, ControllerDelegate& delegate,
          DialogService& dialog_service) -> std::unique_ptr<Controller> {
    return std::move(controller);
  };

  auto* widget = new QWidget;

  EXPECT_CALL(controller_ref, Init(_)).WillOnce(Return(widget));

  auto new_opened_view = std::make_unique<OpenedView>(
      OpenedViewContext{executor_, nullptr, window_definition, dialog_service_,
                        controller_factory});

  EXPECT_CALL(view_manager_delegate_, OnCreateView(_))
      .WillOnce(Return(ByMove(std::move(new_opened_view))));

  auto* opened_view =
      view_manager_qt_.OpenView(window_definition, true, nullptr);

  MockFunction<void()> widget_destroyed;
  QObject::connect(widget, &QObject::destroyed,
                   widget_destroyed.AsStdFunction());

  EXPECT_CALL(widget_destroyed, Call());
  EXPECT_CALL(view_manager_delegate_, OnViewClosed(_));

  view_manager_qt_.CloseView(*opened_view);
}
