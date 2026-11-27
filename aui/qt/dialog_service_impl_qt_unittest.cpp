#include "aui/qt/dialog_service_impl_qt.h"

#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"

#include <QAbstractButton>
#include <QMessageBox>
#include <QPushButton>
#include <gtest/gtest.h>

namespace {

class DialogServiceImplQtTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
  DialogServiceImplQt dialog_service_;
};

}  // namespace

TEST_F(DialogServiceImplQtTest, RunMessageBoxMapsNoButtonResult) {
  auto result =
      scada::aui::qt::test::StartAwaitable(dialog_service_.RunMessageBox(
          u"Confirm?", u"Title", MessageBoxMode::QuestionYesNoDefaultNo));

  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    auto* message_box = qobject_cast<QMessageBox*>(&dialog);
    ASSERT_NE(message_box, nullptr);

    auto* no_button = message_box->button(QMessageBox::No);
    ASSERT_NE(no_button, nullptr);
    no_button->click();
  });

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_EQ(scada::aui::qt::test::GetAwaitableResult(result),
            MessageBoxResult::No);
}

// UX audit 2026-11-25, item 3.7: an apply question names its action and
// defaults to Cancel, so Return does not apply a change by reflex.
TEST_F(DialogServiceImplQtTest, ApplyCancelNamesTheActionAndDefaultsToCancel) {
  auto result =
      scada::aui::qt::test::StartAwaitable(dialog_service_.RunMessageBox(
          u"Apply?", u"Title", MessageBoxMode::QuestionApplyCancel));

  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    auto* message_box = qobject_cast<QMessageBox*>(&dialog);
    ASSERT_NE(message_box, nullptr);
    EXPECT_EQ(message_box->button(QMessageBox::Yes), nullptr);
    QAbstractButton* apply = message_box->button(QMessageBox::Apply);
    QAbstractButton* cancel = message_box->button(QMessageBox::Cancel);
    ASSERT_NE(apply, nullptr);
    ASSERT_NE(cancel, nullptr);
    EXPECT_EQ(static_cast<QAbstractButton*>(message_box->defaultButton()),
              cancel);
    apply->click();
  });

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_EQ(scada::aui::qt::test::GetAwaitableResult(result),
            MessageBoxResult::Apply);
}
