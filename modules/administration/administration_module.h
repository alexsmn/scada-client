#pragma once

class ControllerRegistry;

struct AdministrationModuleContext {
  ControllerRegistry& controller_registry_;
};

// Registers the Administration explorer pane — the left pane of the rail's
// Administration mode (`main_window/pane_modes.cpp`).
//
// The pane is `WIN_REQUIRES_ADMIN`, which is what makes the rail mode itself
// disappear for a non-administrator: `MainWindow::IsPaneModeAvailable` asks the
// command router whether the mode's panes resolve, and the router refuses a
// WIN_REQUIRES_ADMIN command without the Configure right. One rule, enforced
// once.
class AdministrationModule {
 public:
  explicit AdministrationModule(AdministrationModuleContext&& context);
  ~AdministrationModule();

 private:
  AdministrationModuleContext context_;
};
