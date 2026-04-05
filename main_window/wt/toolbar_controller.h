#pragma once

#include "main_window/action_manager.h"

#include <map>

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WToolBar.h>
#pragma warning(pop)

namespace Wt {
class WLayout;
class WMenu;
class WPushButton;
class WToolBar;
}  // namespace Wt

class CommandHandler;
class Executor;

struct ToolbarControllerContext {
  const std::shared_ptr<Executor> executor_;
  ActionManager& action_manager_;
  CommandHandler& commands_;
};

class ToolbarController : private ToolbarControllerContext,
                          private ActionObserver {
 public:
  explicit ToolbarController(ToolbarControllerContext&& context);
  ~ToolbarController();

  std::unique_ptr<Wt::WToolBar> CreateToolbar();

  void OnSelectionChanged();

 private:
  // ActionObserver
  virtual void OnActionChanged(Action& action,
                               ActionChangeMask change_mask) override;

  void UpdateAction(Wt::WPushButton& qaction,
                    unsigned command_id,
                    ActionChangeMask change_mask);

  std::map<unsigned /*command_id*/, Wt::WPushButton*> action_map_;
  std::map<Wt::WPushButton*, unsigned /*command_id*/> action_command_ids_;

  struct CategoryData {
    std::unique_ptr<Wt::WMenu> menu;
    Wt::WPushButton* toolbar_action = nullptr;
  };

  std::map<CommandCategory, CategoryData> category_actions_;
};