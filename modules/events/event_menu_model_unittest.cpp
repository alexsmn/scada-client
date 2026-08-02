#include "events/event_menu_model.h"

#include "aui/models/menu_model.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <set>
#include <vector>

namespace {

// A command handler that records executions and reports configurable
// checked/enabled state, so the menu model can be exercised without a real
// EventView.
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

TEST(EventMenuModelTest, ContainsExpectedItems) {
  FakeCommandHandler commands;
  EventMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int ack = IndexOfCommand(model, ID_ACKNOWLEDGE_CURRENT);
  const int ack_all = IndexOfCommand(model, ID_ACKNOWLEDGE_ALL);
  const int unacked = IndexOfCommand(model, ID_UNACKNOWLEDGED_ONLY);

  ASSERT_GE(ack, 0);
  ASSERT_GE(ack_all, 0);
  ASSERT_GE(unacked, 0);

  EXPECT_EQ(model.GetTypeAt(ack), MenuModel::TYPE_COMMAND);
  EXPECT_EQ(model.GetTypeAt(ack_all), MenuModel::TYPE_COMMAND);
  EXPECT_EQ(model.GetTypeAt(unacked), MenuModel::TYPE_CHECK);

  // Severity is a submenu carrying the two checkable threshold choices.
  int severity_index = -1;
  for (int i = 0; i < model.GetItemCount(); ++i) {
    if (model.GetTypeAt(i) == MenuModel::TYPE_SUBMENU)
      severity_index = i;
  }
  ASSERT_GE(severity_index, 0);

  MenuModel* severity = model.GetSubmenuModelAt(severity_index);
  ASSERT_NE(severity, nullptr);
  EXPECT_GE(IndexOfCommand(*severity, ID_SEVERITY_ALL), 0);
  EXPECT_GE(IndexOfCommand(*severity, ID_SEVERITY_CUSTOM), 0);
  EXPECT_EQ(severity->GetTypeAt(IndexOfCommand(*severity, ID_SEVERITY_ALL)),
            MenuModel::TYPE_CHECK);
}

TEST(EventMenuModelTest, ReflectsCheckedState) {
  FakeCommandHandler commands;
  commands.checked.insert(ID_UNACKNOWLEDGED_ONLY);
  EventMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int unacked = IndexOfCommand(model, ID_UNACKNOWLEDGED_ONLY);
  ASSERT_GE(unacked, 0);
  EXPECT_TRUE(model.IsItemCheckedAt(unacked));
}

TEST(EventMenuModelTest, ReflectsDisabledState) {
  FakeCommandHandler commands;
  commands.disabled.insert(ID_ACKNOWLEDGE_ALL);
  EventMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int ack_all = IndexOfCommand(model, ID_ACKNOWLEDGE_ALL);
  const int ack = IndexOfCommand(model, ID_ACKNOWLEDGE_CURRENT);
  ASSERT_GE(ack_all, 0);
  ASSERT_GE(ack, 0);
  EXPECT_FALSE(model.IsEnabledAt(ack_all));
  EXPECT_TRUE(model.IsEnabledAt(ack));
}

TEST(EventMenuModelTest, ActivatingItemExecutesCommand) {
  FakeCommandHandler commands;
  EventMenuModel menu{commands};
  MenuModel& model = menu.model();

  const int unacked = IndexOfCommand(model, ID_UNACKNOWLEDGED_ONLY);
  ASSERT_GE(unacked, 0);
  model.ActivatedAt(unacked);

  ASSERT_EQ(commands.executed.size(), 1u);
  EXPECT_EQ(commands.executed.front(), ID_UNACKNOWLEDGED_ONLY);
}

}  // namespace
