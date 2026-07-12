#include "modules/table/table_menu_model.h"

#include "aui/models/menu_model.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <set>
#include <vector>

namespace {

// A command handler that records executions and reports configurable
// checked/enabled state, so the menu model can be exercised without a real
// TableView.
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

TEST(TableMenuModelTest, ContainsExpectedItems) {
  FakeCommandHandler commands;
  TableMenuModel menu{commands};
  MenuModel& model = menu.model();

  for (unsigned command_id : {ID_RENAME, ID_MOVE_UP, ID_MOVE_DOWN, ID_DELETE}) {
    const int index = IndexOfCommand(model, command_id);
    ASSERT_GE(index, 0);
    EXPECT_EQ(model.GetTypeAt(index), MenuModel::TYPE_COMMAND);
  }

  // The sort key lives in a submenu carrying the two checkable choices.
  int sort_index = -1;
  for (int i = 0; i < model.GetItemCount(); ++i) {
    if (model.GetTypeAt(i) == MenuModel::TYPE_SUBMENU)
      sort_index = i;
  }
  ASSERT_GE(sort_index, 0);

  MenuModel* sort = model.GetSubmenuModelAt(sort_index);
  ASSERT_NE(sort, nullptr);
  const int name_index = IndexOfCommand(*sort, ID_SORT_NAME);
  const int channel_index = IndexOfCommand(*sort, ID_SORT_CHANNEL);
  ASSERT_GE(name_index, 0);
  ASSERT_GE(channel_index, 0);
  EXPECT_EQ(sort->GetTypeAt(name_index), MenuModel::TYPE_CHECK);
  EXPECT_EQ(sort->GetTypeAt(channel_index), MenuModel::TYPE_CHECK);
}

TEST(TableMenuModelTest, ReflectsCheckedState) {
  FakeCommandHandler commands;
  commands.checked.insert(ID_SORT_NAME);
  TableMenuModel menu{commands};
  MenuModel& model = menu.model();

  int sort_index = -1;
  for (int i = 0; i < model.GetItemCount(); ++i) {
    if (model.GetTypeAt(i) == MenuModel::TYPE_SUBMENU)
      sort_index = i;
  }
  ASSERT_GE(sort_index, 0);
  MenuModel* sort = model.GetSubmenuModelAt(sort_index);
  ASSERT_NE(sort, nullptr);
  EXPECT_TRUE(sort->IsItemCheckedAt(IndexOfCommand(*sort, ID_SORT_NAME)));
  EXPECT_FALSE(sort->IsItemCheckedAt(IndexOfCommand(*sort, ID_SORT_CHANNEL)));
}

TEST(TableMenuModelTest, ReflectsDisabledState) {
  FakeCommandHandler commands;
  commands.disabled.insert(ID_MOVE_UP);
  TableMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int move_up = IndexOfCommand(model, ID_MOVE_UP);
  const int rename = IndexOfCommand(model, ID_RENAME);
  ASSERT_GE(move_up, 0);
  ASSERT_GE(rename, 0);
  EXPECT_FALSE(model.IsEnabledAt(move_up));
  EXPECT_TRUE(model.IsEnabledAt(rename));
}

TEST(TableMenuModelTest, ActivatingItemExecutesCommand) {
  FakeCommandHandler commands;
  TableMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int rename = IndexOfCommand(model, ID_RENAME);
  ASSERT_GE(rename, 0);
  model.ActivatedAt(rename);

  ASSERT_EQ(commands.executed.size(), 1u);
  EXPECT_EQ(commands.executed.front(), ID_RENAME);
}

}  // namespace
