#pragma once

#include "aui/aui_ns_compat.h"

#include "aui/models/simple_menu_model.h"
#include "base/lifetime.h"

class CommandHandler;

// Builds and owns the event view's cross-platform context menu model.
//
// This mirrors the legacy Windows-only `IDR_EVENT_POPUP` resource menu and the
// commands `EventView` contributes (acknowledge, acknowledge-all, severity
// filter, unacknowledged-only filter) as an `aui::MenuModel`, so the same menu
// renders identically on Windows, macOS and Wt through the shared menu-model
// path instead of a `CMenu::LoadMenu` that only exists on Windows.
//
// Checked/enabled state and command execution are delegated to the supplied
// `CommandHandler` (the `EventView`'s command registry).
class EventMenuModel {
 public:
  explicit EventMenuModel(CommandHandler& commands);
  ~EventMenuModel();

  EventMenuModel(const EventMenuModel&) = delete;
  EventMenuModel& operator=(const EventMenuModel&) = delete;

  // The top-level menu model, suitable for the `merge_menu` argument of
  // `ControllerDelegate::ShowPopupMenu`.
  scada::aui::MenuModel& model() SCADA_LIFETIME_BOUND { return model_; }

 private:
  // Routes menu item state queries and activation to the backing command
  // handler, keyed by command id. Mirrors `SimpleMenuCommandHandler` but is
  // kept local so the events module does not depend on `main_window/`.
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

  // Order matters: `delegate_` and `severity_menu_` are referenced by the
  // models constructed after them.
  Delegate delegate_;
  scada::aui::SimpleMenuModel severity_menu_;
  scada::aui::SimpleMenuModel model_;
};
