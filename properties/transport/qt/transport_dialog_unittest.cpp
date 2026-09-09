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
#include <string>
#include <vector>

namespace {

class TestDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  // Records the box and resolves it, so a refused accept() can be asserted
  // on rather than crashing the dialog's error path.
  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                            std::u16string_view title,
                                            MessageBoxMode mode) override {
    messages.emplace_back(message);
    modes.push_back(mode);
    co_return MessageBoxResult::Ok;
  }

  std::vector<std::u16string> messages;
  std::vector<MessageBoxMode> modes;

  Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    throw std::exception{};
  }

  Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    throw std::exception{};
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

  auto result = scada::aui::qt::test::StartAwaitable(
      ShowTransportDialog(dialog_service_, initial));

  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    dialog.findChild<QComboBox*>("typeComboBox")->setCurrentIndex(2);
    dialog.findChild<QLineEdit*>("networkHostLineEdit")
        ->setText("example.test");
    dialog.findChild<QLineEdit*>("networkPortLineEdit")->setText("2404");
    dialog.accept();
  });

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  auto updated = scada::aui::qt::test::GetAwaitableResult(result);
  EXPECT_EQ(updated.ToString(), "UDP;Active;Host=example.test;Port=2404");
}

// Regression (backlog 719): accept() stored text().toInt() straight into the
// port, with no validator, no flag and no range check -- so a typo saved port
// 0 and 70000 was written into the transport string as-is. Now the dialog
// refuses with a message and stays open; a corrected entry then goes through.
TEST_F(TransportDialogTest, OutOfRangePortIsRefusedWithAMessage) {
  transport::TransportString initial;
  initial.SetProtocol(transport::TransportString::TCP);
  initial.SetActive(true);
  initial.SetParam(transport::TransportString::kParamHost, "old-host");
  initial.SetParam(transport::TransportString::kParamPort, 1200);

  auto result = scada::aui::qt::test::StartAwaitable(
      ShowTransportDialog(dialog_service_, initial));

  QDialog* open_dialog = nullptr;
  scada::aui::qt::test::ProcessEventsUntilSettled(result, [&](QDialog& dialog) {
    open_dialog = &dialog;
    // setText bypasses the validator, which is the point: the range check in
    // accept() is what is under test, not QIntValidator.
    dialog.findChild<QLineEdit*>("networkPortLineEdit")->setText("70000");
    dialog.accept();
  });

  ASSERT_NE(open_dialog, nullptr);
  EXPECT_FALSE(scada::aui::qt::test::IsAwaitableReady(result));
  ASSERT_EQ(dialog_service_.messages.size(), 1u);
  EXPECT_EQ(dialog_service_.modes.front(), MessageBoxMode::Error);
  EXPECT_EQ(dialog_service_.messages.front(),
            u"The port must be a number from 1 to 65535.");

  open_dialog->findChild<QLineEdit*>("networkPortLineEdit")->setText("2404");
  open_dialog->accept();
  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog&) {});

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  auto updated = scada::aui::qt::test::GetAwaitableResult(result);
  EXPECT_EQ(updated.ToString(), "TCP;Active;Host=old-host;Port=2404");
}

// An empty port used to parse as 0 and save. Same refusal.
TEST_F(TransportDialogTest, EmptyPortIsRefused) {
  transport::TransportString initial;
  initial.SetProtocol(transport::TransportString::TCP);
  initial.SetActive(true);
  initial.SetParam(transport::TransportString::kParamHost, "old-host");
  initial.SetParam(transport::TransportString::kParamPort, 1200);

  auto result = scada::aui::qt::test::StartAwaitable(
      ShowTransportDialog(dialog_service_, initial));

  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    dialog.findChild<QLineEdit*>("networkPortLineEdit")->clear();
    dialog.accept();
  });

  EXPECT_FALSE(scada::aui::qt::test::IsAwaitableReady(result));
  ASSERT_EQ(dialog_service_.messages.size(), 1u);
  EXPECT_EQ(dialog_service_.modes.front(), MessageBoxMode::Error);
}

TEST_F(TransportDialogTest, RejectedDialogRejectsResult) {
  transport::TransportString initial;
  initial.SetProtocol(transport::TransportString::TCP);
  initial.SetActive(true);
  initial.SetParam(transport::TransportString::kParamHost, "old-host");
  initial.SetParam(transport::TransportString::kParamPort, 1200);

  auto result = scada::aui::qt::test::StartAwaitable(
      ShowTransportDialog(dialog_service_, initial));

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, scada::aui::qt::test::RejectDialog);

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(scada::aui::qt::test::GetAwaitableResult(result),
               std::exception);
}
