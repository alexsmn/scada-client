#include "controller/command_manager.h"

#include "controller/command_ui_registry.h"

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

TEST(CommandManagerTest, ResolveCommandHandlerUsesRegisteredHandler) {
  CommandManager manager;
  manager.RegisterCommand({.command_id = 10, .title = u"Command"});

  TestCommandHandler registered_handler;
  TestCommandHandler fallback_handler;
  manager.RegisterHandler(10, CommandContextId::Global, registered_handler);

  const CommandContextId active_contexts[] = {
      CommandContextId::Global,
  };

  EXPECT_EQ(
      &registered_handler,
      ResolveCommandHandler(manager, 10, active_contexts, fallback_handler));
}

TEST(CommandManagerTest, ResolveCommandHandlerFallsBackToAggregateHandler) {
  CommandManager manager;
  manager.RegisterCommand({.command_id = 10, .title = u"Command"});

  TestCommandHandler fallback_handler;

  const CommandContextId active_contexts[] = {
      CommandContextId::Global,
  };

  EXPECT_EQ(
      &fallback_handler,
      ResolveCommandHandler(manager, 10, active_contexts, fallback_handler));
}

TEST(UiCommandRegistryTest, RegisterCommandStoresPlacements) {
  UiCommandRegistry registry;

  auto& descriptor = registry.RegisterCommand(
      {.command_id = 10, .title = u"Command"},
      {.main_menu = {{.menu_id = MainMenuId::More, .order = 20}},
       .toolbar = false,
       .context_menu = true});

  EXPECT_EQ(10u, descriptor.command_id);
  EXPECT_FALSE(descriptor.show_in_toolbar);
  EXPECT_TRUE(descriptor.show_in_context_menu);

  auto contributions = registry.GetMenuContributions(MainMenuId::More);
  ASSERT_EQ(1u, contributions.size());
  EXPECT_EQ(10u, contributions[0].command_id);
  EXPECT_EQ(20, contributions[0].order);
}

}  // namespace
