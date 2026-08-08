#include "modules/favorites/favourites_view_menu_model.h"

#include "aui/models/menu_model.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <set>
#include <vector>

namespace {

// A command handler that records executions and reports configurable
// checked/enabled state, so the menu model can be exercised without a real
// FavouritesView.
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

TEST(FavouritesViewMenuModelTest, ContainsExpectedItems) {
  FakeCommandHandler commands;
  FavouritesViewMenuModel menu{commands};
  MenuModel& model = menu.model();

  for (unsigned command_id : {ID_OPEN, ID_RENAME, ID_DELETE}) {
    const int index = IndexOfCommand(model, command_id);
    ASSERT_GE(index, 0);
    EXPECT_EQ(model.GetTypeAt(index), MenuModel::TYPE_COMMAND);
  }

#if !defined(UI_WT)
  // Add-web-page is a Qt-only command; the entry must track it.
  EXPECT_GE(IndexOfCommand(model, ID_FAVOURITES_ADD_URL), 0);
#else
  EXPECT_LT(IndexOfCommand(model, ID_FAVOURITES_ADD_URL), 0);
#endif
}

TEST(FavouritesViewMenuModelTest, ReflectsDisabledState) {
  FakeCommandHandler commands;
  commands.disabled.insert(ID_OPEN);
  FavouritesViewMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int open = IndexOfCommand(model, ID_OPEN);
  const int rename = IndexOfCommand(model, ID_RENAME);
  ASSERT_GE(open, 0);
  ASSERT_GE(rename, 0);
  EXPECT_FALSE(model.IsEnabledAt(open));
  EXPECT_TRUE(model.IsEnabledAt(rename));
}

TEST(FavouritesViewMenuModelTest, ActivatingItemExecutesCommand) {
  FakeCommandHandler commands;
  FavouritesViewMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int del = IndexOfCommand(model, ID_DELETE);
  ASSERT_GE(del, 0);
  model.ActivatedAt(del);

  ASSERT_EQ(commands.executed.size(), 1u);
  EXPECT_EQ(commands.executed.front(), ID_DELETE);
}

}  // namespace
