#include "modus/modus_module.h"

#include "aui/test/app_environment.h"
#include "base/check.h"
#include "controller/command_ui_registry.h"
#include "controller/main_menu_window_type_registry.h"
#include "controller/test/controller_environment.h"
#include "filesystem/file_registry.h"
#include "main_window/main_window_interface.h"
#include "modus/modus_component.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>

namespace {

using testing::Contains;
using testing::NotNull;

// `GlobalCommandContext` holds two references, and a global command's handler
// is free to use either. The Modus commands use neither — they capture the
// profile directly — so this stub exists only to make the context
// constructible, and every method fail-stops rather than pretending to work.
// Move it to `client/test/` if a second module's tests need one.
class UnusedMainWindow final : public MainWindowInterface {
 public:
  int GetMainWindowId() const override { scada::base::NotReached(); }
  const Page& GetCurrentPage() const override { scada::base::NotReached(); }
  void OpenPage(const Page&) override { scada::base::NotReached(); }
  void SetCurrentPageTitle(std::u16string_view) override {
    scada::base::NotReached();
  }
  void SaveCurrentPage() override { scada::base::NotReached(); }
  void DeleteCurrentPage() override { scada::base::NotReached(); }
  OpenedViewInterface* GetActiveView() const override {
    scada::base::NotReached();
  }
  OpenedViewInterface* GetActiveDataView() const override {
    scada::base::NotReached();
  }
  void ActivateView(const OpenedViewInterface&) override {
    scada::base::NotReached();
  }
  std::vector<OpenedViewInterface*> GetOpenedViews() const override {
    scada::base::NotReached();
  }
  Awaitable<OpenedViewInterface*> OpenView(const WindowDefinition&,
                                           bool) override {
    scada::base::NotReached();
  }
  OpenedViewInterface* FindViewByType(std::string_view) const override {
    scada::base::NotReached();
  }
  void SplitView(OpenedViewInterface&, bool) override {
    scada::base::NotReached();
  }
};

class ModusModuleTest : public testing::Test {
 protected:
  // Builds the module under test. Deferred rather than constructed as a member
  // so a test can observe the registries both before and after installation —
  // the display-menu registry is process-global, so "was it registered" is only
  // meaningful against a known starting point.
  void InstallModule() {
    module_ = std::make_unique<ModusModule>(
        ModusModuleContext{.controller_registry_ = env_.controller_registry_,
                           .blinker_manager_ = env_.blinker_manager_,
                           .file_registry_ = env_.file_registry_,
                           .global_commands_ = env_.global_commands_,
                           .ui_command_registry_ = ui_command_registry_,
                           .profile_ = env_.profile_});
  }

  // Finds one of the module's global commands by its (untranslated) title. The
  // module lets the registry assign command ids, so the title is the only
  // stable handle a test has.
  const BasicCommand<GlobalCommandContext>* FindGlobalCommand(
      std::u16string_view title) const {
    for (const auto& command : env_.global_commands_.commands()) {
      if (command.title == title)
        return &command;
    }
    return nullptr;
  }

  GlobalCommandContext command_context() {
    return {.main_window = main_window_,
            .dialog_service = env_.dialog_service_};
  }

  AppEnvironment app_env_;
  ControllerEnvironment env_;
  UiCommandRegistry ui_command_registry_;
  UnusedMainWindow main_window_;
  std::unique_ptr<ModusModule> module_;
};

TEST_F(ModusModuleTest, RegistersAControllerFactoryForTheModusWindowType) {
  // `GetControllerFactory` returns a std::function by value, so the question
  // is whether it is empty rather than whether it is null.
  ASSERT_FALSE(static_cast<bool>(env_.controller_registry_.GetControllerFactory(
      kModusWindowInfo.command_id)));

  InstallModule();

  EXPECT_TRUE(static_cast<bool>(env_.controller_registry_.GetControllerFactory(
      kModusWindowInfo.command_id)));
}

TEST_F(ModusModuleTest, RegistersBothModusDocumentExtensions) {
  InstallModule();

  const FileRegistry::TypeEntry* entry =
      env_.file_registry_.FindTypeById(kModusWindowInfo.command_id);

  ASSERT_THAT(entry, NotNull());
  EXPECT_EQ(entry->name, kModusWindowInfo.name);
  // The registry splits on ';' and keeps the leading dot, so these are the
  // literal tokens `ModusModule` passed, not bare extensions.
  EXPECT_THAT(entry->extensions,
              testing::UnorderedElementsAre(".sde", ".xsde"));
}

TEST_F(ModusModuleTest, ContributesItsWindowTypeToTheDisplayMenu) {
  InstallModule();

  EXPECT_THAT(GetDisplayMenuWindowTypes(), Contains(kModusWindowInfo.name));
}

// The display-menu registry is process-global, so a module that registers on
// construction and forgets to unregister on destruction leaves a menu entry
// pointing at a controller factory that has gone away. Pinning the pair is the
// point: the destructor's `UnregisterDisplayMenuWindowType` has no other guard.
TEST_F(ModusModuleTest, WithdrawsItsWindowTypeWhenDestroyed) {
  InstallModule();
  ASSERT_THAT(GetDisplayMenuWindowTypes(), Contains(kModusWindowInfo.name));

  module_.reset();

  EXPECT_THAT(GetDisplayMenuWindowTypes(),
              testing::Not(Contains(kModusWindowInfo.name)));
}

TEST_F(ModusModuleTest, TopologyCommandTracksAndTogglesTheProfileFlag) {
  InstallModule();

  const auto* command = FindGlobalCommand(u"Show Modus topology");
  ASSERT_THAT(command, NotNull());
  ASSERT_TRUE(command->execute_handler);
  ASSERT_TRUE(command->checked_handler);

  const GlobalCommandContext context = command_context();

  env_.profile_.modus.topology = false;
  EXPECT_FALSE(command->checked_handler(context));

  command->execute_handler(context);
  EXPECT_TRUE(env_.profile_.modus.topology);
  EXPECT_TRUE(command->checked_handler(context));

  command->execute_handler(context);
  EXPECT_FALSE(env_.profile_.modus.topology);
  EXPECT_FALSE(command->checked_handler(context));
}

// DEFECT (task 483): this command toggles and persists `profile.modus.modus2`,
// and **nothing reads it**. Its only reader is `IsModus2` in
// `modules/modus/modus_util.cpp`, which is itself uncalled from anywhere in the
// tree (measured 2026-08-25). So the operator gets a checkable menu item that
// changes no rendering. The toggle is pinned here as it behaves today; whether
// the fix is to wire the flag up or to withdraw the command is task 483.
TEST_F(ModusModuleTest, RuntimeRendererCommandTogglesAFlagNothingConsumes) {
  InstallModule();

  const auto* command = FindGlobalCommand(u"Use Modus runtime renderer");
  ASSERT_THAT(command, NotNull());

  const GlobalCommandContext context = command_context();

  env_.profile_.modus.modus2 = false;
  EXPECT_FALSE(command->checked_handler(context));

  command->execute_handler(context);

  EXPECT_TRUE(env_.profile_.modus.modus2);
  EXPECT_TRUE(command->checked_handler(context));
}

TEST_F(ModusModuleTest, BothGlobalCommandsSitInTheDisplaySettingsGroup) {
  InstallModule();

  for (std::u16string_view title :
       {u"Show Modus topology", u"Use Modus runtime renderer"}) {
    const auto* command = FindGlobalCommand(title);
    ASSERT_THAT(command, NotNull());
    ASSERT_TRUE(command->menu_group.has_value());
    EXPECT_EQ(*command->menu_group, MenuGroup::DISPLAY_SETTINGS);
  }
}

TEST_F(ModusModuleTest, RegistersTheSetupAction) {
  InstallModule();

  EXPECT_THAT(ui_command_registry_.action_manager().FindAction(ID_SETUP),
              NotNull());
}

}  // namespace
