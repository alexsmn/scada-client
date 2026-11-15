#include "main_window/command_palette_launcher.h"

#include "aui/test/app_environment.h"
#include "base/test/test_executor.h"
#include "controller/command_manager.h"
#include "main_window/command_palette_qt.h"
#include "node_service/test/fake_node_service.h"
#include "scada/session_service_mock.h"

#include <QLineEdit>
#include <QPointer>
#include <QWidget>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace {

class CommandPaletteLauncherTest : public ::testing::Test {
 protected:
  std::unique_ptr<CommandPaletteLauncher> MakeLauncher() {
    return std::make_unique<CommandPaletteLauncher>(
        CommandPaletteLauncherContext{
            .executor_ = executor_,
            .command_manager_ = command_manager_,
            .resolve_command_ = [](unsigned) -> CommandHandler* {
              return nullptr;
            },
            .open_tag_ = [](const scada::NodeId&, const std::u16string&) {},
            .parent_ = parent_});
  }

  std::unique_ptr<CommandPaletteLauncher> MakeLauncherWithSession() {
    return std::make_unique<CommandPaletteLauncher>(
        CommandPaletteLauncherContext{
            .executor_ = executor_,
            .command_manager_ = command_manager_,
            .resolve_command_ = [](unsigned) -> CommandHandler* {
              return nullptr;
            },
            .open_tag_ = [](const scada::NodeId&, const std::u16string&) {},
            .parent_ = parent_,
            .node_service_ = &node_service_,
            .session_service_ = &session_service_});
  }

  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;
  TestExecutor executor_;
  CommandManager command_manager_;
  QWidget parent_;
  FakeNodeService node_service_;
  testing::NiceMock<scada::MockSessionService> session_service_;
};

// No address space means a palette of commands alone — and starting the
// browse is then a no-op rather than a crash.
TEST_F(CommandPaletteLauncherTest, WithoutANodeServiceThereIsNoTagSearch) {
  std::unique_ptr<CommandPaletteLauncher> launcher = MakeLauncher();

  EXPECT_FALSE(launcher->has_tag_search());
  launcher->StartTagSearchBrowse();
}

// A third-party server has no DataItemType tags; the walk would browse its
// whole Organizes tree and find nothing (backlog 1054). The answer follows the
// session, so a re-login onto our server brings tag search back.
TEST_F(CommandPaletteLauncherTest, AForeignServerHasNoTagSearch) {
  bool scada = false;
  ON_CALL(session_service_, IsScada()).WillByDefault([&] { return scada; });
  std::unique_ptr<CommandPaletteLauncher> launcher = MakeLauncherWithSession();

  EXPECT_FALSE(launcher->has_tag_search());
  launcher->StartTagSearchBrowse();
  EXPECT_TRUE(node_service_.fetch_requests(scada::id::ObjectsFolder).empty());

  scada = true;
  EXPECT_TRUE(launcher->has_tag_search());
}

TEST_F(CommandPaletteLauncherTest, ShowParentsThePaletteToTheHost) {
  std::unique_ptr<CommandPaletteLauncher> launcher = MakeLauncher();

  CommandPalette* palette = launcher->Show({});

  ASSERT_NE(palette, nullptr);
  EXPECT_EQ(palette->parentWidget(), &parent_);
  EXPECT_TRUE(palette->testAttribute(Qt::WA_DeleteOnClose));
}

// Type-to-search from the context bar: the characters typed into the field
// arrive as the palette's filter, so the typing continues uninterrupted.
TEST_F(CommandPaletteLauncherTest, ShowSeedsTheFilterWithTheTypedText) {
  std::unique_ptr<CommandPaletteLauncher> launcher = MakeLauncher();

  CommandPalette* palette = launcher->Show(QStringLiteral("pum"));

  auto* filter = palette->findChild<QLineEdit*>();
  ASSERT_NE(filter, nullptr);
  EXPECT_EQ(filter->text(), QStringLiteral("pum"));
}

// The palette deletes itself on close, so a host holding it can never keep a
// dangling pointer past the operator dismissing it.
TEST_F(CommandPaletteLauncherTest, ClosingThePaletteDeletesIt) {
  std::unique_ptr<CommandPaletteLauncher> launcher = MakeLauncher();
  QPointer<CommandPalette> palette = launcher->Show({});

  palette->close();
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

  EXPECT_TRUE(palette.isNull());
}

}  // namespace
