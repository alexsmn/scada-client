#pragma once

#include "events/severity_tiles.h"

#include <QWidget>

#include <functional>

class QLabel;
class QTimer;

namespace events {
class SeverityTileStrip;
}

// The context bar's right slot: the alarm state, and nothing else.
//
// Left to right, as shell-chrome.html draws it: the escalation ladder's two
// rungs (events/alarm_escalation.h), then the per-severity KPI tiles. A chip
// states that the operator must act now where a tile states a count, which is
// why the chips come first.
//
// - Rung 1, the annunciator: at least one unacknowledged critical alarm
//   (ISA-18.2). It flashes while lit; the audible half is EventDispatcher's.
// - Rung 2, the flood pill: the unacknowledged count has crossed the flood
//   threshold, so a flood reads as a state rather than a scroll.
//
// Both rungs and the tiles are derived from one read of the counts per
// Refresh, so the chips and the tiles beside them cannot disagree about the
// same alarms. Split out of MainWindow on 2026-09-27 (backlog 722).
class AlarmStateCluster : public QWidget {
 public:
  // Read afresh on every Refresh().
  using CountsProvider = std::function<events::SeverityTileCounts()>;

  explicit AlarmStateCluster(CountsProvider counts, QWidget* parent = nullptr);
  ~AlarmStateCluster() override;

  // Re-reads the counts and brings the chips and tiles in line with them. The
  // host calls it whenever the alarm set changes.
  void Refresh();

  // The chips, for tests and captures; always non-null. Visible only while
  // their rung is lit.
  QLabel* annunciator() const { return annunciator_; }
  QLabel* flood_pill() const { return flood_pill_; }
  // Whether the annunciator is in the filled half of its flash. Meaningful
  // only while it is lit.
  bool annunciator_flash_on() const { return annunciator_flash_on_; }

 private:
  // Repaints the annunciator chip for the current phase of its flash.
  void StyleAnnunciator();

  const CountsProvider counts_;

  QLabel* annunciator_ = nullptr;
  // Runs only while the annunciator is lit.
  QTimer* annunciator_flash_ = nullptr;
  bool annunciator_flash_on_ = false;
  QLabel* flood_pill_ = nullptr;
  // Null if the tile builder produced no strip.
  events::SeverityTileStrip* severity_tiles_ = nullptr;
};
