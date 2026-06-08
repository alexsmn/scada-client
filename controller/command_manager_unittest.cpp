#include "controller/command_manager.h"

#include <gtest/gtest.h>

namespace {

class TestCommandHandler : public CommandHandler {
 public:
  explicit TestCommandHandler(bool available = true) : available_{available} {}

  virtual CommandHandler* GetCommandHandler(unsigned command_id) override {
    return available_ ? this : nullptr;
  }

 private:
  bool available_ = true;
};

TEST(CommandManagerTest, RegisterCommandPreservesInsertionOrder) {
  CommandManager manager;

  manager.RegisterCommand({.command_id = 10, .title = u"First"});
  manager.RegisterCommand({.command_id = 20, .title = u"Second"});

  ASSERT_EQ(2u, manager.commands().size());
  EXPECT_EQ(10u, manager.commands()[0]->command_id);
  EXPECT_EQ(20u, manager.commands()[1]->command_id);
}

TEST(CommandManagerTest, RegisterCommandReturnsExistingDescriptor) {
  CommandManager manager;

  auto& first = manager.RegisterCommand({.command_id = 10, .title = u"First"});
  auto& second =
      manager.RegisterCommand({.command_id = 10, .title = u"Duplicate"});

  EXPECT_EQ(&first, &second);
  EXPECT_EQ(u"First", second.title);
  EXPECT_EQ(1u, manager.commands().size());
}

TEST(CommandManagerTest, ResolveHandlerUsesHighestActiveContext) {
  CommandManager manager;
  manager.RegisterCommand({.command_id = 10, .title = u"Command"});

  TestCommandHandler global_handler;
  TestCommandHandler opened_view_handler;
  manager.RegisterHandler(10, CommandContextId::Global, global_handler);
  manager.RegisterHandler(10, CommandContextId::OpenedView,
                          opened_view_handler);

  const CommandContextId active_contexts[] = {
      CommandContextId::Global,
      CommandContextId::OpenedView,
  };

  EXPECT_EQ(&opened_view_handler, manager.ResolveHandler(10, active_contexts));
}

TEST(CommandManagerTest, ResolveHandlerSkipsUnavailableContext) {
  CommandManager manager;
  manager.RegisterCommand({.command_id = 10, .title = u"Command"});

  TestCommandHandler global_handler;
  TestCommandHandler opened_view_handler{/*available=*/false};
  manager.RegisterHandler(10, CommandContextId::Global, global_handler);
  manager.RegisterHandler(10, CommandContextId::OpenedView,
                          opened_view_handler);

  const CommandContextId active_contexts[] = {
      CommandContextId::Global,
      CommandContextId::OpenedView,
  };

  EXPECT_EQ(&global_handler, manager.ResolveHandler(10, active_contexts));
}

}  // namespace
