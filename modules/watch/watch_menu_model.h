#pragma once

#include "aui/models/simple_menu_model.h"
#include "base/lifetime.h"

class CommandHandler;

// Builds and owns the watch (log) view's cross-platform context menu model.
//
// This mirrors the legacy Windows-only `IDR_LOG_POPUP` resource menu for the
// commands `WatchView` contributes (a checkable pause toggle, save-as and
// clear) as an `aui::MenuModel`, so the same menu renders identically on
// Windows, macOS and Wt through the shared menu-model path instead of a
// `CMenu::LoadMenu` that only exists on Windows.
//
// Checked/enabled state and command execution are delegated to the supplied
// `CommandHandler` (the `WatchView`'s command registry).
class WatchMenuModel {
 public:
  explicit WatchMenuModel(CommandHandler& commands);
  ~WatchMenuModel();

  WatchMenuModel(const WatchMenuModel&) = delete;
  WatchMenuModel& operator=(const WatchMenuModel&) = delete;

  // The top-level menu model, suitable for the `merge_menu` argument of
  // `ControllerDelegate::ShowPopupMenu`.
  scada::aui::MenuModel& model() SCADA_LIFETIME_BOUND { return model_; }

 private:
  // Routes menu item state queries and activation to the backing command
  // handler, keyed by command id. Mirrors `SimpleMenuCommandHandler` but is
  // kept local so the watch module does not depend on `main_window/`.
  class Delegate : public scada::aui::SimpleMenuModel::Delegate {
   public:
    explicit Delegate(CommandHandler& commands) : commands_{commands} {}

    // scada::aui::SimpleMenuModel::Delegate
    virtual bool IsCommandIdChecked(int command_id) const override;
    virtual bool IsCommandIdEnabled(int command_id) const override;
    virtual void ExecuteCommand(int command_id) override;

   private:
    CommandHandler& commands_;
  };

  // Order matters: `delegate_` is referenced by the model constructed after it.
  Delegate delegate_;
  scada::aui::SimpleMenuModel model_;
};
