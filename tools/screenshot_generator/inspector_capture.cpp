#include "inspector_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "aui/translation.h"
#include "inspector/qt/inspector_panel.h"
#include "scada/event.h"

#include <QString>

namespace {

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

}  // namespace

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
  panel.ShowEvent(
      QStringLiteral("КП-03 МЭК-61850"), QStringLiteral("TS.105"),
      QStringLiteral("КП-03: обрыв связи"), scada::kSeverityCritical,
      QStringLiteral("16.04.2026 14:17:00"), QStringLiteral("— ожидает —"),
      /*acknowledgeable=*/true,
      /*source_available=*/true);
  SaveScreenshot(&panel, spec);
}

void SaveInspectorScreenshot(const ScreenshotSpec& spec) {
  // A representative analog signal selection: a live value with its configured
  // limit bands, currently above the Hi warning limit — so the capture shows
  // both the Measurements limits block and the marked breach that explains the
  // value's colouring. The control action is disabled with its reason, which
  // documents the disabled affordance.
  InspectorPanel panel{InspectorPanelContext{}};
  panel.ShowElement(InspectorElementView{
      .title = QStringLiteral("Напряжение Ua"),
      .node_id_text = QStringLiteral("TIT.723"),
      .value_text = QStringLiteral("10,9 кВ"),
      .quality = InspectorQualityBand::kGood,
      .updated_text = QStringLiteral("15:32:20"),
      // The band labels go through Translate() exactly as the live panel does
      // (ShowElement takes them already rendered), so the capture shows the
      // shipped Russian wording rather than the English keys.
      .limits = {{.label = Tr("HiHi"), .value = QStringLiteral("11,5")},
                 {.label = Tr("Hi"),
                  .value = QStringLiteral("10,8"),
                  .breached = true},
                 {.label = Tr("Lo"), .value = QStringLiteral("9,5")},
                 {.label = Tr("LoLo"), .value = QStringLiteral("9,0")}},
      .controllable = false,
      .control_reason = Tr("The signal has no output channel")});
  SaveScreenshot(&panel, spec);
}
