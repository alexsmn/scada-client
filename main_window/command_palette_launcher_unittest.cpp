#include "main_window/command_palette_launcher.h"

#include "aui/test/app_environment.h"
#include "base/test/test_executor.h"
#include "controller/command_manager.h"
#include "main_window/command_palette_qt.h"

#include <QLineEdit>
#include <QPointer>
#include <QWidget>

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

  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;
  TestExecutor executor_;
  CommandManager command_manager_;
  QWidget parent_;
};

// No address space means a palette of commands alone — and starting the
// browse is then a no-op rather than a crash.
TEST_F(CommandPaletteLauncherTest, WithoutANodeServiceThereIsNoTagSearch) {
  std::unique_ptr<CommandPaletteLauncher> launcher = MakeLauncher();

  EXPECT_FALSE(launcher->has_tag_search());
  launcher->StartTagSearchBrowse();
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
