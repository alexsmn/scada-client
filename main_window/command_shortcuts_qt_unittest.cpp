#include "main_window/command_shortcuts_qt.h"

#include "aui/test/app_environment.h"
#include "controller/command_handler.h"
#include "controller/command_manager.h"

#include <QKeySequence>
#include <QShortcut>
#include <QWidget>

#include <gmock/gmock.h>

#include <set>
#include <vector>

using namespace testing;

namespace {

constexpr unsigned kCopy = 10;
constexpr unsigned kDelete = 11;
constexpr unsigned kNoShortcut = 12;

// A handler that answers for the commands the test says it does, with an
// enabled flag the test can flip, and records what it was asked to run.
class FakeHandler : public CommandHandler {
 public:
  bool IsCommandEnabled(unsigned command_id) const override {
    return !disabled_.contains(command_id);
  }
  void ExecuteCommand(unsigned command_id) override {
    executed_.push_back(command_id);
  }

  std::set<unsigned> disabled_;
  std::vector<unsigned> executed_;
};

class CommandShortcutsTest : public Test {
 protected:
  CommandShortcutsTest() {
    command_manager_.RegisterCommand(
        {.command_id = kCopy,
         .title = u"Copy",
         .shortcut =
             Shortcut{scada::aui::ControlModifier, scada::aui::KeyCode::C}});
    command_manager_.RegisterCommand(
        {.command_id = kDelete,
         .title = u"Delete",
         .shortcut = Shortcut{scada::aui::KeyCode::Delete}});
    command_manager_.RegisterCommand(
        {.command_id = kNoShortcut, .title = u"Properties"});
  }

  CommandShortcuts MakeShortcuts() {
    return CommandShortcuts{command_manager_,
                            [this](unsigned command_id) -> CommandHandler* {
                              return resolving_.contains(command_id) ? &handler_
                                                                     : nullptr;
                            },
                            window_};
  }

  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;
  CommandManager command_manager_;
  QWidget window_;
  FakeHandler handler_;
  std::set<unsigned> resolving_;
};

TEST_F(CommandShortcutsTest, OneShortcutPerCommandThatDeclaresOne) {
  CommandShortcuts shortcuts = MakeShortcuts();

  EXPECT_EQ(shortcuts.size(), 2u);
  ASSERT_NE(shortcuts.Find(kCopy), nullptr);
  ASSERT_NE(shortcuts.Find(kDelete), nullptr);
  EXPECT_EQ(shortcuts.Find(kNoShortcut), nullptr);
  EXPECT_EQ(shortcuts.Find(kCopy)->key(), QKeySequence(Qt::CTRL | Qt::Key_C));
  EXPECT_EQ(shortcuts.Find(kDelete)->key(), QKeySequence(Qt::Key_Delete));
  EXPECT_EQ(shortcuts.Find(kCopy)->context(), Qt::WindowShortcut);
  EXPECT_EQ(shortcuts.Find(kCopy)->parent(), &window_);
}

// A shortcut that is enabled has consumed its key whether or not anything then
// runs, so it must be off whenever the command does not apply — that is what
// leaves Ctrl+C to a text field or a grid.
TEST_F(CommandShortcutsTest,
       EnabledExactlyWhileTheCommandResolvesAndIsEnabled) {
  CommandShortcuts shortcuts = MakeShortcuts();
  EXPECT_FALSE(shortcuts.Find(kCopy)->isEnabled())
      << "nothing resolves before the first Refresh";

  shortcuts.Refresh();
  EXPECT_FALSE(shortcuts.Find(kCopy)->isEnabled());

  resolving_ = {kCopy, kDelete};
  handler_.disabled_ = {kDelete};
  shortcuts.Refresh();
  EXPECT_TRUE(shortcuts.Find(kCopy)->isEnabled());
  EXPECT_FALSE(shortcuts.Find(kDelete)->isEnabled());

  resolving_.clear();
  shortcuts.Refresh();
  EXPECT_FALSE(shortcuts.Find(kCopy)->isEnabled());
}

TEST_F(CommandShortcutsTest, ActivatingRunsTheCommand) {
  CommandShortcuts shortcuts = MakeShortcuts();
  resolving_ = {kDelete};
  shortcuts.Refresh();

  emit shortcuts.Find(kDelete)->activated();

  EXPECT_THAT(handler_.executed_, ElementsAre(kDelete));
}

// The enabled state is only as fresh as the last selection change, so the
// command is re-checked on activation and a stale enable never runs anything.
TEST_F(CommandShortcutsTest, AStaleEnableRunsNothing) {
  CommandShortcuts shortcuts = MakeShortcuts();
  resolving_ = {kDelete};
  shortcuts.Refresh();

  handler_.disabled_ = {kDelete};
  emit shortcuts.Find(kDelete)->activated();
  resolving_.clear();
  emit shortcuts.Find(kDelete)->activated();

  EXPECT_THAT(handler_.executed_, IsEmpty());
}

// The shortcuts belong to the window but call back into the registry, so they
// must not outlive it.
TEST_F(CommandShortcutsTest, DestroyingTheRegistryRemovesItsShortcuts) {
  {
    CommandShortcuts shortcuts = MakeShortcuts();
    EXPECT_EQ(window_.findChildren<QShortcut*>().size(), 2);
  }
  EXPECT_TRUE(window_.findChildren<QShortcut*>().isEmpty());
}

}  // namespace
