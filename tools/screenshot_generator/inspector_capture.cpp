#include "inspector_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "inspector/qt/inspector_panel.h"

#include <QString>

void SaveInspectorEventScreenshot(const ScreenshotSpec& spec) {
  // A representative journal-row selection: a pending critical comms-loss
  // alarm, matching the fixture's seeded event 1008 — severity band pill,
  // message, event time at the frozen fixture clock, "— pending —" state and
  // the live Acknowledge + To-graph actions. Severity colours are
  // severity-theme-gated, so this capture renders under --theme
  // (reshell-theme tag).
  InspectorPanel panel{
      InspectorPanelContext{.is_acknowledge_enabled = [] { return true; },
                            .is_go_to_source_enabled = [] { return true; }}};
  panel.ShowEvent(QStringLiteral("КП-03 МЭК-61850"), QStringLiteral("TS.105"),
                  QStringLiteral("КП-03: обрыв связи"),
                  /*severity=*/80, QStringLiteral("16.04.2026 14:17:00"),
                  QStringLiteral("— ожидает —"),
                  /*acknowledgeable=*/true,
                  /*source_available=*/true);
  SaveScreenshot(&panel, spec);
}

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
