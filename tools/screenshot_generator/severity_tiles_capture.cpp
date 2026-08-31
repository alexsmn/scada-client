#include "severity_tiles_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "events/qt/severity_tile_strip.h"

#include <memory>

void SaveSeverityTilesScreenshot(const ScreenshotSpec& spec) {
  // A plausible mid-incident picture: one critical and three warnings active,
  // with a larger unacknowledged backlog (some of it already returned to
  // normal) — which is exactly the state the separate unacknowledged tile
  // exists to show.
  const events::SeverityTileCounts counts{
      .critical = 1, .warning = 3, .unacknowledged = 7};

  std::unique_ptr<events::SeverityTileStrip> strip{
      events::MakeSeverityTileStrip([counts] { return counts; })};
  SaveScreenshot(strip.get(), spec);
}
