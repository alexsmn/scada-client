#include "main_window/command_actions_qt.h"

#include "aui/test/app_environment.h"
#include "controller/command_handler.h"
#include "controller/command_manager.h"

#include <QAction>
#include <QMenu>
#include <QObject>

#include <memory>
#include <set>

#include <gtest/gtest.h>

namespace {

constexpr unsigned kOnButtons = 101;
constexpr unsigned kOffButtons = 102;
constexpr unsigned kCheckable = 103;

// A handler whose answers the test sets directly.
class FakeHandler : public CommandHandler {
 public:
  bool IsCommandEnabled(unsigned command_id) const override { return enabled; }
  bool IsCommandChecked(unsigned command_id) const override { return checked; }
  void ExecuteCommand(unsigned command_id) override {
    executed.insert(command_id);
  }

  bool enabled = true;
  bool checked = false;
  std::set<unsigned> executed;
};

class CommandActionsTest : public ::testing::Test {
 protected:
  CommandActionsTest() {
    command_manager_.RegisterCommand({.command_id = kOnButtons,
                                      .title = u"Long title",
                                      .short_title = u"Short"});
    command_manager_.RegisterCommand({.command_id = kOffButtons,
                                      .title = u"Menu only",
                                      .show_in_toolbar = false});
    command_manager_.RegisterCommand({.command_id = kCheckable,
                                      .title = u"Toggle",
                                      .flags = Action::CHECKABLE});
  }

  // Builds the registry under test. `resolves` decides which commands the
  // window currently has a handler for.
  std::unique_ptr<CommandActions> MakeActions() {
    return std::make_unique<CommandActions>(
        command_manager_, action_manager_,
        [this](unsigned command_id) -> CommandHandler* {
          return resolves.contains(command_id) ? &handler : nullptr;
        },
        parent_);
  }

  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;
  CommandManager command_manager_;
  ActionManager action_manager_;
  QObject parent_;

  FakeHandler handler;
  std::set<unsigned> resolves;
};

TEST_F(CommandActionsTest, CreatesAnActionOnlyForButtonSurfaceCommands) {
  auto actions = MakeActions();

  EXPECT_EQ(actions->size(), 2u);
  ASSERT_NE(actions->Find(kOnButtons), nullptr);
  EXPECT_EQ(actions->Find(kOnButtons)->text(), QStringLiteral("Short"));
  EXPECT_EQ(actions->Find(kOffButtons), nullptr);
  ASSERT_NE(actions->Find(kCheckable), nullptr);
  EXPECT_TRUE(actions->Find(kCheckable)->isCheckable());
  EXPECT_FALSE(actions->Find(kOnButtons)->isCheckable());
}

// The point of the split (backlog 697): the actions exist, parented to the
// window, with no toolbar anywhere.
TEST_F(CommandActionsTest, ActionsNeedNoToolbar) {
  auto actions = MakeActions();

  EXPECT_EQ(actions->Find(kOnButtons)->parent(), &parent_);
  EXPECT_TRUE(actions->Find(kOnButtons)->associatedObjects().isEmpty());
}

TEST_F(CommandActionsTest, StartsHiddenAndFollowsTheResolvedHandler) {
  auto actions = MakeActions();
  QAction* action = actions->Find(kCheckable);
  EXPECT_FALSE(action->isVisible());

  resolves = {kCheckable};
  handler.checked = true;
  actions->UpdateAll();
  EXPECT_TRUE(action->isVisible());
  EXPECT_TRUE(action->isEnabled());
  EXPECT_TRUE(action->isChecked());

  handler.enabled = false;
  actions->UpdateAll();
  EXPECT_TRUE(action->isVisible());
  EXPECT_FALSE(action->isEnabled());

  resolves.clear();
  actions->UpdateAll();
  EXPECT_FALSE(action->isVisible());
}

// Triggering re-resolves rather than trusting the action's enabled flag, so an
// action whose state went stale cannot execute a command the handler refuses.
TEST_F(CommandActionsTest, TriggerExecutesOnlyAnEnabledResolvedCommand) {
  auto actions = MakeActions();
  QAction* action = actions->Find(kOnButtons);

  action->trigger();
  EXPECT_TRUE(handler.executed.empty());

  resolves = {kOnButtons};
  handler.enabled = false;
  action->trigger();
  EXPECT_TRUE(handler.executed.empty());

  handler.enabled = true;
  action->trigger();
  EXPECT_EQ(handler.executed, std::set<unsigned>{kOnButtons});
}

TEST_F(CommandActionsTest, TitleChangeNotificationRetitlesTheAction) {
  Action source;
  source.command_id_ = kOnButtons;
  source.title_ = u"Renamed";
  action_manager_.AddAction(source);
  auto actions = MakeActions();

  // UpdateAll deliberately leaves titles alone.
  actions->UpdateAll();
  EXPECT_EQ(actions->Find(kOnButtons)->text(), QStringLiteral("Short"));

  action_manager_.NotifyActionChanged(kOnButtons, ActionChangeMask::All);
  EXPECT_EQ(actions->Find(kOnButtons)->text(), QStringLiteral("Renamed"));
}

TEST_F(CommandActionsTest, UpdateMenuTouchesOnlyItsOwnActions) {
  auto actions = MakeActions();
  QMenu menu;
  menu.addAction(actions->Find(kOnButtons));
  QAction* foreign = menu.addAction(QStringLiteral("Foreign"));
  foreign->setVisible(true);

  resolves = {kOnButtons};
  actions->UpdateMenu(menu);

  EXPECT_TRUE(actions->Find(kOnButtons)->isVisible());
  EXPECT_TRUE(foreign->isVisible());
  // The other registered action is not in the menu and is not updated.
  EXPECT_FALSE(actions->Find(kCheckable)->isVisible());
}

// Destroying the registry stops it following notifications; the actions stay
// with their parent.
TEST_F(CommandActionsTest, DestructionLeavesTheActionsWithTheirParent) {
  Action source;
  source.command_id_ = kOnButtons;
  source.title_ = u"Renamed";
  action_manager_.AddAction(source);
  auto actions = MakeActions();
  QAction* action = actions->Find(kOnButtons);

  actions.reset();
  action_manager_.NotifyActionChanged(kOnButtons, ActionChangeMask::All);

  EXPECT_EQ(action->parent(), &parent_);
  EXPECT_EQ(action->text(), QStringLiteral("Short"));
}

}  // namespace
