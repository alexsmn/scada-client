#include "components/time_range/time_range_dialog.h"

#include "aui/dialog_service.h"
#include "aui/test/app_environment.h"
#include "profile/profile.h"

#include <QApplication>
#include <QDateTime>
#include <QDialog>
#include <QEventLoop>
#include <gtest/gtest.h>

#include <thread>

namespace {

class TestDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  promise<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                          std::u16string_view title,
                                          MessageBoxMode mode) override {
    return make_rejected_promise<MessageBoxResult>(std::exception{});
  }

  promise<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    return make_rejected_promise<std::filesystem::path>(std::exception{});
  }

  promise<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    return make_rejected_promise<std::filesystem::path>(std::exception{});
  }
};

void ProcessEventsUntilSettled(promise<TimeRange>& result) {
  for (int i = 0; i < 200 &&
                  result.wait_for(std::chrono::milliseconds{0}) ==
                      promise_wait_status::timeout;
       ++i) {
    QApplication::processEvents(QEventLoop::AllEvents |
                                    QEventLoop::WaitForMoreEvents,
                                20);
    if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
      dialog->accept();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
}

class TimeRangeDialogTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
  TestDialogService dialog_service_;
  Profile profile_;
};

base::Time ToBaseTime(const QDateTime& date_time) {
  return base::Time::UnixEpoch() +
         base::TimeDelta::FromMilliseconds(date_time.toMSecsSinceEpoch());
}

}  // namespace

TEST_F(TimeRangeDialogTest, AcceptedDialogReturnsSelectedInitialRange) {
  const TimeRange initial_range{
      ToBaseTime({QDate{2024, 1, 2}, QTime{0, 0}}),
      ToBaseTime({QDate{2024, 1, 3}, QTime{0, 0}}), /*dates=*/true};

  auto result = ShowTimeRangeDialog(
      dialog_service_, TimeRangeContext{profile_, initial_range,
                                        /*time_required_=*/false});
  ProcessEventsUntilSettled(result);

  ASSERT_NE(result.wait_for(std::chrono::milliseconds{0}),
            promise_wait_status::timeout);
  auto selected_range = result.get();
  EXPECT_TRUE(selected_range.dates);
  EXPECT_EQ(selected_range.start, initial_range.start);
  EXPECT_EQ(selected_range.end,
            initial_range.end + base::TimeDelta::FromDays(1));
}
