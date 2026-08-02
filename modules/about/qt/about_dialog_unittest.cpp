#include "modules/about/about_dialog.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QDialog>

namespace {

// Regression: ShowAboutDialog handed its dialog to a lazy Awaitable-returning
// helper and discarded the result, so the never-started coroutine destroyed
// the dialog and Help → About… silently did nothing.
TEST(AboutDialogTest, ShowAboutDialogShowsTheDialog) {
  AppEnvironment app_env;
  DialogServiceImplQt dialog_service;

  ShowAboutDialog(dialog_service);
  QApplication::processEvents();

  QDialog* visible = nullptr;
  for (QWidget* widget : QApplication::topLevelWidgets()) {
    if (widget->isVisible()) {
      if (auto* dialog = qobject_cast<QDialog*>(widget))
        visible = dialog;
    }
  }
  ASSERT_NE(visible, nullptr);

  visible->reject();
  // The dialog deleteLater's itself on finish; flush the DeferredDelete
  // event so the fixture tears down with no dangling top-level widget.
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

}  // namespace
