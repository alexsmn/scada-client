#include "screenshot_modules.h"

ClientApplicationModules ScreenshotModuleSelection() {
  // The shipping client's module set, so every command-driven surface — menus,
  // toolbars, context menus — renders the rows an operator sees, in the order
  // the operator sees them (a context menu lists its rows in
  // command-registration order).
  //
  // This used to be a hand-maintained list mirroring
  // `MakeDefaultClientApplicationModules`. Views come from REGISTER_CONTROLLER
  // and so render whichever modules are installed, but a module that is not
  // installed contributes no commands: the menus that would list them came out
  // short and nothing failed. Every menu capture added since 2026-08-15 had to
  // add its module here by hand when somebody noticed.
  ClientApplicationModules modules;
  // Both draw through Windows ActiveX/COM controls. No `--capture` renders
  // through them (the `manual-modus` family is hand-maintained), and leaving
  // them out keeps a capture's command rows identical on every host.
  modules.modus = false;
  modules.vidicon = false;
  return modules;
}

ClientApplicationModuleConfigurator MakeScreenshotModules() {
  return MakeDefaultClientApplicationModules(ScreenshotModuleSelection());
}
