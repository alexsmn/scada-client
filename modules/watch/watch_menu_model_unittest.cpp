#include "modules/watch/watch_menu_model.h"

#include "aui/models/menu_model.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <set>
#include <vector>

namespace {

// A command handler that records executions and reports configurable
// checked/enabled state, so the menu model can be exercised without a real
// WatchView.
class FakeCommandHandler : public CommandHandler {
 public:
  std::set<unsigned> checked;
  std::set<unsigned> disabled;
  std::vector<unsigned> executed;

  virtual bool IsCommandChecked(unsigned command_id) const override {
    return checked.contains(command_id);
  }
  virtual bool IsCommandEnabled(unsigned command_id) const override {
    return !disabled.contains(command_id);
  }
  virtual void ExecuteCommand(unsigned command_id) override {
    executed.push_back(command_id);
  }
};

using scada::aui::MenuModel;

// Returns the index of the item with the given command id, or -1.
int IndexOfCommand(const MenuModel& menu, unsigned command_id) {
  for (int i = 0; i < menu.GetItemCount(); ++i) {
    if (menu.GetCommandIdAt(i) == static_cast<int>(command_id))
      return i;
  }
  return -1;
}

TEST(WatchMenuModelTest, ContainsExpectedItems) {
  FakeCommandHandler commands;
  WatchMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int pause = IndexOfCommand(model, ID_PAUSE);
  const int save_as = IndexOfCommand(model, ID_SAVE_AS);
  const int clear = IndexOfCommand(model, ID_CLEAR_ALL);

  ASSERT_GE(pause, 0);
  ASSERT_GE(save_as, 0);
  ASSERT_GE(clear, 0);

  // Pause is checkable; save-as and clear are plain commands.
  EXPECT_EQ(model.GetTypeAt(pause), MenuModel::TYPE_CHECK);
  EXPECT_EQ(model.GetTypeAt(save_as), MenuModel::TYPE_COMMAND);
  EXPECT_EQ(model.GetTypeAt(clear), MenuModel::TYPE_COMMAND);
}

TEST(WatchMenuModelTest, ReflectsCheckedState) {
  FakeCommandHandler commands;
  commands.checked.insert(ID_PAUSE);
  WatchMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int pause = IndexOfCommand(model, ID_PAUSE);
  ASSERT_GE(pause, 0);
  EXPECT_TRUE(model.IsItemCheckedAt(pause));
}

TEST(WatchMenuModelTest, ReflectsDisabledState) {
  FakeCommandHandler commands;
  commands.disabled.insert(ID_SAVE_AS);
  WatchMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int save_as = IndexOfCommand(model, ID_SAVE_AS);
  const int clear = IndexOfCommand(model, ID_CLEAR_ALL);
  ASSERT_GE(save_as, 0);
  ASSERT_GE(clear, 0);
  EXPECT_FALSE(model.IsEnabledAt(save_as));
  EXPECT_TRUE(model.IsEnabledAt(clear));
}

TEST(WatchMenuModelTest, ActivatingItemExecutesCommand) {
  FakeCommandHandler commands;
  WatchMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int pause = IndexOfCommand(model, ID_PAUSE);
  ASSERT_GE(pause, 0);
  model.ActivatedAt(pause);

  ASSERT_EQ(commands.executed.size(), 1u);
  EXPECT_EQ(commands.executed.front(), ID_PAUSE);
}

}  // namespace
