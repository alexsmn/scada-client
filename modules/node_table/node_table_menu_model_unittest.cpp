#include "modules/node_table/node_table_menu_model.h"

#include "aui/models/menu_model.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <set>
#include <vector>

namespace {

// A command handler that records executions and reports configurable
// checked/enabled state, so the menu model can be exercised without a real
// NodeTableController.
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

// Returns the index of the (single) submenu item, or -1.
int IndexOfSubmenu(const MenuModel& menu) {
  for (int i = 0; i < menu.GetItemCount(); ++i) {
    if (menu.GetTypeAt(i) == MenuModel::TYPE_SUBMENU)
      return i;
  }
  return -1;
}

TEST(NodeTableMenuModelTest, ContainsExpectedItems) {
  FakeCommandHandler commands;
  NodeTableMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int rename = IndexOfCommand(model, ID_RENAME);
  ASSERT_GE(rename, 0);
  EXPECT_EQ(model.GetTypeAt(rename), MenuModel::TYPE_COMMAND);

  // The sort key lives in a submenu carrying the three checkable choices the
  // legacy IDR_GRID_POPUP offered.
  const int sort_index = IndexOfSubmenu(model);
  ASSERT_GE(sort_index, 0);

  MenuModel* sort = model.GetSubmenuModelAt(sort_index);
  ASSERT_NE(sort, nullptr);
  for (unsigned command_id : {ID_SORT_NONE, ID_SORT_ALIAS, ID_SORT_CHANNEL}) {
    const int index = IndexOfCommand(*sort, command_id);
    ASSERT_GE(index, 0);
    EXPECT_EQ(sort->GetTypeAt(index), MenuModel::TYPE_CHECK);
  }
}

TEST(NodeTableMenuModelTest, ReflectsCheckedState) {
  FakeCommandHandler commands;
  commands.checked.insert(ID_SORT_ALIAS);
  NodeTableMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int sort_index = IndexOfSubmenu(model);
  ASSERT_GE(sort_index, 0);
  MenuModel* sort = model.GetSubmenuModelAt(sort_index);
  ASSERT_NE(sort, nullptr);
  EXPECT_TRUE(sort->IsItemCheckedAt(IndexOfCommand(*sort, ID_SORT_ALIAS)));
  EXPECT_FALSE(sort->IsItemCheckedAt(IndexOfCommand(*sort, ID_SORT_NONE)));
  EXPECT_FALSE(sort->IsItemCheckedAt(IndexOfCommand(*sort, ID_SORT_CHANNEL)));
}

TEST(NodeTableMenuModelTest, ReflectsDisabledState) {
  FakeCommandHandler commands;
  commands.disabled.insert(ID_RENAME);
  NodeTableMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int rename = IndexOfCommand(model, ID_RENAME);
  ASSERT_GE(rename, 0);
  EXPECT_FALSE(model.IsEnabledAt(rename));
}

TEST(NodeTableMenuModelTest, ActivatingItemExecutesCommand) {
  FakeCommandHandler commands;
  NodeTableMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int rename = IndexOfCommand(model, ID_RENAME);
  ASSERT_GE(rename, 0);
  model.ActivatedAt(rename);

  ASSERT_EQ(commands.executed.size(), 1u);
  EXPECT_EQ(commands.executed.front(), ID_RENAME);
}

TEST(NodeTableMenuModelTest, ActivatingSortItemExecutesCommand) {
  FakeCommandHandler commands;
  NodeTableMenuModel menu{commands};
  MenuModel& model = menu.model();

  MenuModel* sort = model.GetSubmenuModelAt(IndexOfSubmenu(model));
  ASSERT_NE(sort, nullptr);
  const int channel = IndexOfCommand(*sort, ID_SORT_CHANNEL);
  ASSERT_GE(channel, 0);
  sort->ActivatedAt(channel);

  ASSERT_EQ(commands.executed.size(), 1u);
  EXPECT_EQ(commands.executed.front(), ID_SORT_CHANNEL);
}

}  // namespace
