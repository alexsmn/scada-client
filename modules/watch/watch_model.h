#pragma once

#include "aui/models/table_model.h"
#include "base/lifetime.h"
#include "base/relative_time_range.h"
#include "modules/watch/device_log_line.h"
#include "modules/watch/watch_event_source.h"
#include "node_service/node_ref.h"
#include "scada/event.h"
#include "timed_data/timed_data_view.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

class NodeService;

struct WatchModelContext {
  NodeService& node_service_;
  WatchEventSource& event_source_;
};

// What the device log view is showing. Both modes read the same device-watch
// event stream; the frame trace narrows it to the lines the drivers marked as
// protocol traffic (see device_log_line.h) and surfaces their direction.
enum class WatchMode {
  // Everything the device reported, as logged.
  kLog,
  // Protocol traffic only, with a direction column.
  kFrameTrace,
};

// What the view shows within the current mode. The defaults pass everything,
// so an unfiltered log is the starting state.
//
// The filter applies in both modes rather than only in the frame trace. A
// filter that keeps applying while its control is hidden is a trap, and
// text-filtering a device log is useful in its own right; `kind` simply
// excludes every row that is not a frame of that format.
struct WatchFilter {
  // Link-layer format, as the trace mockup's All / I-format / S+U segments.
  enum class Kind {
    kAny,
    // I-format: the frames that carry an ASDU, i.e. actual data.
    kInformation,
    // S- and U-format: acknowledgements and link control. Grouped because
    // neither carries data and both matter for the same question — why the
    // link is not moving.
    kSupervisoryAndUnnumbered,
  };

  bool operator==(const WatchFilter&) const = default;

  Kind kind = Kind::kAny;
  // Warnings and worse only — usually the reason the view was opened at all.
  bool errors_only = false;
  // Substring, matched against every column: IOA, type, cause, the sequence
  // numbers and the message. Case-insensitive for ASCII, which is what these
  // columns hold.
  std::u16string text;
};

class WatchModel : private WatchModelContext,
                   public scada::aui::TableModel,
                   protected WatchEventSource::Delegate {
 public:
  explicit WatchModel(WatchModelContext&& context);

  // One logged line. `frame` is present when the server reported structured
  // frame data (DeviceFrameEventType); it is absent for ordinary log lines and
  // for any server that predates it, which is why the marker heuristic in
  // device_log_line.h is still consulted as a fallback.
  struct Row {
    scada::Event event;
    std::optional<scada::DeviceFrame> frame;
  };


  // The row shown at visible index `row`, or nullptr when `row` is out of
  // range — which includes "nothing is selected" (-1), so callers need no
  // separate guard. The decode pane reads the selected frame through this.
  const Row* FindVisibleRow(int row) const SCADA_LIFETIME_BOUND;

  const NodeRef& device() const SCADA_LIFETIME_BOUND { return device_; }
  void SetDevice(NodeRef device);

  const scada::RelativeTimeRange& time_range() const SCADA_LIFETIME_BOUND {
    return time_range_;
  }
  void SetTimeRange(const scada::RelativeTimeRange& time_range);

  WatchMode mode() const { return mode_; }
  void SetMode(WatchMode mode);

  const WatchFilter& filter() const SCADA_LIFETIME_BOUND { return filter_; }
  void SetFilter(WatchFilter filter);

  bool paused() const { return paused_; }
  void set_paused(bool paused) { paused_ = paused; }

  void Clear();

  void SaveLog(const std::filesystem::path& path);

  // ui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(scada::aui::TableCell& cell) override;

 protected:
  // WatchEventSource
  virtual void OnEvent(const scada::Event& event) override;
  virtual void OnDeviceFrame(const scada::DeviceFrameEvent& event) override;
  virtual void OnError(const scada::Status& status) override;

 private:
  void AddLine(Row row);
  // The text of one cell, without the row having to be visible — the filter
  // matches against columns while it is deciding which rows exist, so it
  // cannot go through GetCell.
  std::u16string CellText(const Row& row, int column_id) const;
  // Whether `row` survives the current mode and filter.
  bool IsVisible(const Row& row) const;
  bool MatchesFilterText(const Row& row) const;
  // Swaps the visible row set wholesale, notifying the view. Used by the mode
  // and filter setters: both are deliberate operator actions on a bounded log,
  // so a rebuild is cheaper to get right than a diff.
  void ReplaceVisible();
  // Recomputes `visible_` for the current mode. Cheap enough to run wholesale:
  // the log is already bounded by the time range.
  void RebuildVisible();
  // The event at visible row `row`.
  const Row& VisibleRow(int row) const SCADA_LIFETIME_BOUND;

  // The direction of `row`: from the structured frame when the server sent one,
  // otherwise from the message's #/$ marker.
  DeviceLogDirection DirectionOf(const Row& row) const;

  NodeRef device_;

  WatchMode mode_ = WatchMode::kLog;

  WatchFilter filter_;

  // Indices into `events_` that the current mode shows.
  std::vector<int> visible_;

  scada::RelativeTimeRange time_range_{std::chrono::minutes(15)};

  // Sorted by `scada::Event::time`.
  std::vector<Row> events_;

  bool paused_ = false;
};
