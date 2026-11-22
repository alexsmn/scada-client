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
  // Vidicon draws through a Windows ActiveX/COM control and is built on
  // Windows only, so leaving it out keeps a capture's command rows identical
  // on every host.
  //
  // Modus stays IN. It was switched off on the same reasoning until
  // 2026-11-22, but `ModusController` builds a `ModusDisplayView` over the
  // linked display runtime (ADR 0013), not ActiveX, and the module is built on
  // every client host. Leaving it out had a visible cost: `ModusModule` is
  // what registers the .sde/.xsde file type and the display-menu window type,
  // so the menu bar's «Мнемосхема» menu rendered with no rows at all, not even
  // its <No displays> placeholder, and menu-scheme.png could not be captured.
  modules.vidicon = false;
  return modules;
}

ClientApplicationModuleConfigurator MakeScreenshotModules() {
  return MakeDefaultClientApplicationModules(ScreenshotModuleSelection());
}
