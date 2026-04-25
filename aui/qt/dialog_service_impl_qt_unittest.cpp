#include "aui/qt/dialog_service_impl_qt.h"

#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"

#include <QAbstractButton>
#include <QMessageBox>
#include <gtest/gtest.h>

namespace {

class DialogServiceImplQtTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
  DialogServiceImplQt dialog_service_;
};

}  // namespace

TEST_F(DialogServiceImplQtTest, RunMessageBoxMapsNoButtonResult) {
  auto result = dialog_service_.RunMessageBox(
      u"Confirm?", u"Title", MessageBoxMode::QuestionYesNoDefaultNo);

  aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    auto* message_box = qobject_cast<QMessageBox*>(&dialog);
    ASSERT_NE(message_box, nullptr);

    auto* no_button = message_box->button(QMessageBox::No);
    ASSERT_NE(no_button, nullptr);
    no_button->click();
  });

  ASSERT_TRUE(aui::qt::test::IsPromiseReady(result));
  EXPECT_EQ(result.get(), MessageBoxResult::No);
}
