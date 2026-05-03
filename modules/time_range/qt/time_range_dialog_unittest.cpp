#include "modules/time_range/time_range_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"
#include "profile/profile.h"

#include <QDateTime>
#include <QDialog>
#include <gtest/gtest.h>

namespace {

class TestDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                            std::u16string_view title,
                                            MessageBoxMode mode) override {
    throw std::exception{};
    co_return MessageBoxResult::Ok;
  }

  Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    throw std::exception{};
    co_return std::filesystem::path{};
  }

  Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    throw std::exception{};
    co_return std::filesystem::path{};
  }
};

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

  auto result = aui::qt::test::StartAwaitable(ShowTimeRangeDialog(
      dialog_service_, TimeRangeContext{profile_, initial_range,
                                        /*time_required_=*/false}));
  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::AcceptDialog);

  ASSERT_TRUE(aui::qt::test::IsAwaitableReady(result));
  auto selected_range = aui::qt::test::GetAwaitableResult(result);
  EXPECT_TRUE(selected_range.dates);
  EXPECT_EQ(selected_range.start, initial_range.start);
  EXPECT_EQ(selected_range.end,
            initial_range.end + base::TimeDelta::FromDays(1));
}
