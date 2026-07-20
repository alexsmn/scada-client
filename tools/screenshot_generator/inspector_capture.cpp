#include "inspector_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "inspector/qt/inspector_panel.h"

#include <QString>

void SaveInspectorScreenshot(const ScreenshotSpec& spec) {
  // A representative Table expression-row selection: a live computed value
  // identified by its formula, with good quality and a fresh update. The
  // control action is disabled — an expression is not commandable — which
  // also documents the disabled affordance.
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(QStringLiteral("Полная мощность"),
                    QStringLiteral("{TIT.200}+{TIT.201}"),
                    QStringLiteral("128,7"), InspectorQualityBand::kGood,
                    QStringLiteral("15:32:20"),
                    /*controllable=*/false);
  SaveScreenshot(&panel, spec);
}
