#include "modules/table/qt/table_toolbar.h"

#include "aui/test/app_environment.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

#include <QToolButton>
#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

// Records executions and exposes settable enabled/checked state.
class FakeCommandHandler : public CommandHandler {
 public:
  virtual bool IsCommandEnabled(unsigned command_id) const override {
    return enabled;
  }
  virtual bool IsCommandChecked(unsigned command_id) const override {
    return checked;
  }
  virtual void ExecuteCommand(unsigned command_id) override {
    executed.push_back(command_id);
  }

  bool enabled = true;
  bool checked = false;
  std::vector<unsigned> executed;
};

QToolButton* CommandButton(TableToolbar& bar, unsigned command_id) {
  return bar.findChild<QToolButton*>(
      QStringLiteral("tableToolbarCmd%1").arg(command_id));
}

class TableToolbarTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
};

// A resolved command shows as a button and executes through its handler; a
// command nothing resolves (unavailable in this session) hides its button.
TEST_F(TableToolbarTest, ButtonsFollowResolutionAndExecute) {
  FakeCommandHandler handler;
  std::unique_ptr<TableToolbar> bar{MakeTableToolbar(TableToolbarContext{
      .resolve_command = [&](unsigned command_id) -> CommandHandler* {
        return command_id == ID_DELETE ? &handler : nullptr;
      }})};
  ASSERT_NE(bar, nullptr);

  QToolButton* delete_button = CommandButton(*bar, ID_DELETE);
  ASSERT_NE(delete_button, nullptr);
  EXPECT_FALSE(delete_button->isHidden());
  EXPECT_TRUE(delete_button->isEnabled());

  QToolButton* print_button = CommandButton(*bar, ID_PRINT);
  ASSERT_NE(print_button, nullptr);
  EXPECT_TRUE(print_button->isHidden());

  delete_button->click();
  EXPECT_EQ(handler.executed, std::vector<unsigned>{ID_DELETE});
}

// Refresh() re-reads enablement, and a disabled button does not execute.
TEST_F(TableToolbarTest, RefreshTracksEnablement) {
  FakeCommandHandler handler;
  std::unique_ptr<TableToolbar> bar{MakeTableToolbar(
      TableToolbarContext{.resolve_command = [&](unsigned) -> CommandHandler* {
        return &handler;
      }})};
  ASSERT_NE(bar, nullptr);

  QToolButton* delete_button = CommandButton(*bar, ID_DELETE);
  ASSERT_NE(delete_button, nullptr);
  EXPECT_TRUE(delete_button->isEnabled());

  handler.enabled = false;
  bar->Refresh();
  EXPECT_FALSE(delete_button->isEnabled());

  delete_button->click();
  EXPECT_TRUE(handler.executed.empty());
}

// The sort-key buttons mirror the handler's checked state.
TEST_F(TableToolbarTest, SortKeysMirrorCheckedState) {
  FakeCommandHandler handler;
  std::unique_ptr<TableToolbar> bar{MakeTableToolbar(
      TableToolbarContext{.resolve_command = [&](unsigned) -> CommandHandler* {
        return &handler;
      }})};
  ASSERT_NE(bar, nullptr);

  QToolButton* sort_name = CommandButton(*bar, ID_SORT_NAME);
  ASSERT_NE(sort_name, nullptr);
  EXPECT_FALSE(sort_name->isChecked());

  handler.checked = true;
  bar->Refresh();
  EXPECT_TRUE(sort_name->isChecked());
}

// Add-signal is the view's own affordance and fires its callback directly.
TEST_F(TableToolbarTest, AddSignalFiresCallback) {
  bool added = false;
  std::unique_ptr<TableToolbar> bar{MakeTableToolbar(
      TableToolbarContext{.on_add_signal = [&] { added = true; }})};
  ASSERT_NE(bar, nullptr);

  auto* add = bar->findChild<QToolButton*>(QStringLiteral("tableToolbarAdd"));
  ASSERT_NE(add, nullptr);
  add->click();
  EXPECT_TRUE(added);
}

}  // namespace
