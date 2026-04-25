#include "properties/transport/transport_dialog.h"

#include "aui/dialog_service.h"
#include "aui/test/app_environment.h"

#include <transport/transport_string.h>

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QEventLoop>
#include <QLineEdit>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
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

template <class T, class DialogAction>
void ProcessEventsUntilSettled(promise<T>& result, DialogAction action) {
  bool acted = false;
  for (int i = 0; i < 200 &&
                  result.wait_for(std::chrono::milliseconds{0}) ==
                      promise_wait_status::timeout;
       ++i) {
    QApplication::processEvents(QEventLoop::AllEvents |
                                    QEventLoop::WaitForMoreEvents,
                                20);
    if (!acted) {
      if (auto* dialog =
              qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
        action(*dialog);
        acted = true;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
}

class TransportDialogTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
  TestDialogService dialog_service_;
};

}  // namespace

TEST_F(TransportDialogTest, AcceptedDialogReturnsEditedTransportString) {
  transport::TransportString initial;
  initial.SetProtocol(transport::TransportString::TCP);
  initial.SetActive(true);
  initial.SetParam(transport::TransportString::kParamHost, "old-host");
  initial.SetParam(transport::TransportString::kParamPort, 1200);

  auto result = ShowTransportDialog(dialog_service_, initial);

  ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    dialog.findChild<QComboBox*>("typeComboBox")->setCurrentIndex(2);
    dialog.findChild<QLineEdit*>("networkHostLineEdit")
        ->setText("example.test");
    dialog.findChild<QLineEdit*>("networkPortLineEdit")->setText("2404");
    dialog.accept();
  });

  ASSERT_NE(result.wait_for(std::chrono::milliseconds{0}),
            promise_wait_status::timeout);
  auto updated = result.get();
  EXPECT_EQ(updated.ToString(), "UDP;Active;Host=example.test;Port=2404");
}

TEST_F(TransportDialogTest, RejectedDialogRejectsResult) {
  transport::TransportString initial;
  initial.SetProtocol(transport::TransportString::TCP);
  initial.SetActive(true);
  initial.SetParam(transport::TransportString::kParamHost, "old-host");
  initial.SetParam(transport::TransportString::kParamPort, 1200);

  auto result = ShowTransportDialog(dialog_service_, initial);

  ProcessEventsUntilSettled(result, [](QDialog& dialog) { dialog.reject(); });

  ASSERT_NE(result.wait_for(std::chrono::milliseconds{0}),
            promise_wait_status::timeout);
  EXPECT_THROW(result.get(), std::exception);
}
