#include "modules/sheet/sheet_menu_model.h"

#include "aui/models/menu_model.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <set>
#include <vector>

namespace {

// A command handler that records executions and reports configurable
// checked/enabled state, so the menu model can be exercised without a real
// SheetController.
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

TEST(SheetMenuModelTest, ContainsExpectedItems) {
  FakeCommandHandler commands;
  SheetMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int color = IndexOfCommand(model, ID_GRAPH_COLOR);
  ASSERT_GE(color, 0);
  EXPECT_EQ(model.GetTypeAt(color), MenuModel::TYPE_COMMAND);
}

TEST(SheetMenuModelTest, ReflectsDisabledState) {
  FakeCommandHandler commands;
  commands.disabled.insert(ID_GRAPH_COLOR);
  SheetMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int color = IndexOfCommand(model, ID_GRAPH_COLOR);
  ASSERT_GE(color, 0);
  EXPECT_FALSE(model.IsEnabledAt(color));
}

TEST(SheetMenuModelTest, ActivatingItemExecutesCommand) {
  FakeCommandHandler commands;
  SheetMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int color = IndexOfCommand(model, ID_GRAPH_COLOR);
  ASSERT_GE(color, 0);
  model.ActivatedAt(color);

  ASSERT_EQ(commands.executed.size(), 1u);
  EXPECT_EQ(commands.executed.front(), ID_GRAPH_COLOR);
}

}  // namespace
