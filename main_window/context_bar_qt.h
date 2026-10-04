#pragma once

#include "main_window/alarm_state_cluster_qt.h"
#include "main_window/breadcrumb_qt.h"

#include <QKeySequence>
#include <QToolBar>

#include <functional>
#include <span>

class CommandField;

// The top context bar: three slots, as `operator-shell.html` lays the bar out.
// Left, where the workspace is (a breadcrumb: page → view → selected object);
// centre, the command/search field that opens the Ctrl-K palette; right, alarm
// state alone (AlarmStateCluster). No brand mark and no identity or
// connection cells — the window title names the application and the status
// strip owns who/where (docs/client/ux/shell.md §2.2).
//
// Split out of MainWindow on 2026-10-04 (backlog 722). The bar knows nothing
// of pages, views or selections: the host computes the breadcrumb and hands it
// over, and tells the bar when the alarm counts may have moved. The object
// names `ContextBar` and `contextBar{Left,Centre,Right}Slot` are what the
// screenshot generator's shell assertions look up, so they must not change.
class ContextBar final : public QToolBar {
 public:
  // Opens the command palette, seeded with `initial_text` when the operator
  // started typing into the field.
  using OpenPaletteCallback = std::function<void(const QString& initial_text)>;

  ContextBar(AlarmStateCluster::CountsProvider alarm_counts,
             OpenPaletteCallback open_palette,
             QWidget* parent);
  ~ContextBar() override;

  // The palette's shortcut, which the field shows as its hint. The host binds
  // it window-wide; the bar only names it.
  static QKeySequence PaletteKey();

  // Replaces the breadcrumb's segments.
  void SetBreadcrumb(std::span<const Breadcrumb::Segment> segments);

  // Re-reads the alarm counts and brings the chips and tiles in line.
  void RefreshAlarmState();

 private:
  const OpenPaletteCallback open_palette_;

  Breadcrumb* breadcrumb_ = nullptr;
  CommandField* command_field_ = nullptr;
  AlarmStateCluster* alarm_state_ = nullptr;
};
