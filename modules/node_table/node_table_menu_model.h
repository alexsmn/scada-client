#pragma once

#include "aui/models/simple_menu_model.h"
#include "base/lifetime.h"

class CommandHandler;

// Builds and owns the node table (grid) view's cross-platform context menu
// model.
//
// This mirrors the legacy Windows-only `IDR_GRID_POPUP` resource menu for the
// commands `NodeTableController` contributes (rename and the sort-by submenu)
// as an `aui::MenuModel`, so the same menu renders identically on Windows
// and macOS through the shared menu-model path instead of a
// `CMenu::LoadMenu` that only exists on Windows. The node-specific commands the
// resource menu carried via its `<Item>` placeholder are supplied by the
// generic context menu the shell appends.
//
// Checked/enabled state and command execution are delegated to the supplied
// `CommandHandler` (the `NodeTableController`'s command registry).
class NodeTableMenuModel {
 public:
  explicit NodeTableMenuModel(CommandHandler& commands);
  ~NodeTableMenuModel();

  NodeTableMenuModel(const NodeTableMenuModel&) = delete;
  NodeTableMenuModel& operator=(const NodeTableMenuModel&) = delete;

  // The top-level menu model, suitable for the `merge_menu` argument of
  // `ControllerDelegate::ShowPopupMenu`.
  scada::aui::MenuModel& model() SCADA_LIFETIME_BOUND { return model_; }

 private:
  // Routes menu item state queries and activation to the backing command
  // handler, keyed by command id. Mirrors `SimpleMenuCommandHandler` but is
  // kept local so the node table module does not depend on `main_window/`.
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

  // Order matters: `delegate_` and `sort_menu_` are referenced by the models
  // constructed after them.
  Delegate delegate_;
  scada::aui::SimpleMenuModel sort_menu_;
  scada::aui::SimpleMenuModel model_;
};
