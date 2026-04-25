#include "properties/transport/transport_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"

#include <transport/transport_string.h>

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <gtest/gtest.h>

#include <filesystem>

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

  aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    dialog.findChild<QComboBox*>("typeComboBox")->setCurrentIndex(2);
    dialog.findChild<QLineEdit*>("networkHostLineEdit")
        ->setText("example.test");
    dialog.findChild<QLineEdit*>("networkPortLineEdit")->setText("2404");
    dialog.accept();
  });

  ASSERT_TRUE(aui::qt::test::IsPromiseReady(result));
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

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::RejectDialog);

  ASSERT_TRUE(aui::qt::test::IsPromiseReady(result));
  EXPECT_THROW(result.get(), std::exception);
}
