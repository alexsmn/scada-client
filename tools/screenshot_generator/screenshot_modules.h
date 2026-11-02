#pragma once

#include "app/client_application_modules.h"

// The modules the screenshot generator installs: the shipping client's
// standard set, minus the ActiveX-backed Modus and Vidicon modules.
ClientApplicationModules ScreenshotModuleSelection();

// Returns the module configurator the screenshot fixture hands to
// `ClientApplication` — `MakeDefaultClientApplicationModules` over
// `ScreenshotModuleSelection()`.
ClientApplicationModuleConfigurator MakeScreenshotModules();
