#pragma once

#include "events/severity_tiles.h"

#include <QWidget>

#include <functional>
#include <vector>

class QLabel;

namespace events {

// Supplies the live tile counts; read afresh on every Refresh() so the strip
// never caches a stale alarm picture.
using SeverityTileCountsProvider = std::function<SeverityTileCounts()>;

// The KPI severity strip (UX backlog 2.3): the "how bad is it right now" tiles
// — Critical / Warning / Unacknowledged — rendered from BuildSeverityTiles(),
// so display order (most severe first) and colouring come from the shared
// severity source rather than from this widget.
//
// A tile reads as calm when its count is zero (plain, muted) and asserts itself
// when it is not (bold, severity-coloured). Unacknowledged carries no severity
// colour: it is a workflow state, not a severity. The caption always states
// what the number means, so colour is never the only signal.
//
// Opt-in reshell chrome — build it through MakeSeverityTileStrip(), which
// returns nullptr under the legacy theme.
class SeverityTileStrip : public QWidget {
 public:
  explicit SeverityTileStrip(SeverityTileCountsProvider counts,
                             QWidget* parent = nullptr);

  // Re-reads the counts and restyles the tiles. Cheap — the host calls it
  // whenever the alarm set changes.
  void Refresh();

 private:
  SeverityTileCountsProvider counts_;
  std::vector<QLabel*> tiles_;
};

// Builds the strip, or returns nullptr under the legacy severity theme so the
// default UI is unchanged (the same gating the other reshell surfaces use).
SeverityTileStrip* MakeSeverityTileStrip(SeverityTileCountsProvider counts,
                                         QWidget* parent = nullptr);

}  // namespace events
