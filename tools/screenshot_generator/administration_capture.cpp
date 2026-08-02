#include "administration_capture.h"

#include "widget_capture.h"

#include "administration/administration_sections.h"
#include "administration/qt/administration_panel.h"

void SaveAdministrationScreenshot(const ScreenshotSpec& spec) {
  AdministrationPanel panel;
  // Every candidate section: the capture stands for an administrator holding
  // the Configure right, for whom the shell resolves all of them.
  panel.ShowSections(GetAdministrationSections());
  panel.resize(214, 400);

  SaveScreenshot(&panel, spec);
}
