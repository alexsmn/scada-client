#include "modules/write/qt/control_review_dialog.h"

#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <gtest/gtest.h>

namespace {

ControlReview MakeReview() {
  return ControlReview{.title = u"Control",
                       .object = u"Breaker Q1",
                       .present = u"On",
                       .command = u"Off",
                       .operator_name = u"Operator Ivanov",
                       .time = u"23.11.2026 14:00:00",
                       .stage = u"One stage",
                       .warning = u"Cannot be undone remotely."};
}

// Whether `dialog` shows a label reading exactly `text`.
bool ShowsText(const QDialog& dialog, const std::u16string& text) {
  for (const QLabel* label : dialog.findChildren<QLabel*>()) {
    if (label->text() == QString::fromStdU16String(text))
      return true;
  }
  return false;
}

class ControlReviewDialogTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
};

}  // namespace

// Backlog 1118: the review replaced a «Да/Нет» question box. It must show
// every field of the review, not just a question.
TEST_F(ControlReviewDialogTest, ShowsEveryFieldOfTheReview) {
  const ControlReview review = MakeReview();
  ControlReviewDialog dialog{review};

  EXPECT_EQ(dialog.windowTitle(), QStringLiteral("Control"));
  for (const std::u16string* field :
       {&review.object, &review.present, &review.command, &review.operator_name,
        &review.time, &review.stage, &review.warning}) {
    EXPECT_TRUE(ShowsText(dialog, *field))
        << QString::fromStdU16String(*field).toStdString();
  }
}

// dialogs.md §3 and §4: the accept button names the action, never «Yes», and
// Cancel is the default so Return does not send an irreversible command.
TEST_F(ControlReviewDialogTest, NamesTheActionAndDefaultsToCancel) {
  ControlReviewDialog dialog{MakeReview()};
  dialog.show();
  QApplication::processEvents();

  QPushButton* send = dialog.button_box()->button(QDialogButtonBox::Ok);
  QPushButton* cancel = dialog.button_box()->button(QDialogButtonBox::Cancel);
  ASSERT_NE(send, nullptr);
  ASSERT_NE(cancel, nullptr);
  EXPECT_EQ(send->text(), QStringLiteral("Send command"));
  EXPECT_EQ(dialog.button_box()->buttonRole(send),
            QDialogButtonBox::AcceptRole);
  EXPECT_TRUE(cancel->isDefault());
  EXPECT_FALSE(send->isDefault());
  EXPECT_FALSE(send->autoDefault());
  dialog.reject();
}

TEST_F(ControlReviewDialogTest, SendingAnswersTrue) {
  auto result = scada::aui::qt::test::StartAwaitable(
      RunControlReviewDialog(nullptr, MakeReview()));
  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    auto* review = qobject_cast<ControlReviewDialog*>(&dialog);
    ASSERT_NE(review, nullptr);
    review->button_box()->button(QDialogButtonBox::Ok)->click();
  });
  ASSERT_TRUE(result->value.has_value());
  EXPECT_TRUE(*result->value);
}

TEST_F(ControlReviewDialogTest, CancellingAnswersFalse) {
  auto result = scada::aui::qt::test::StartAwaitable(
      RunControlReviewDialog(nullptr, MakeReview()));
  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, [](QDialog& dialog) { dialog.reject(); });
  ASSERT_TRUE(result->value.has_value());
  EXPECT_FALSE(*result->value);
}
