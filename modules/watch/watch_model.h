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


  const NodeRef& device() const SCADA_LIFETIME_BOUND { return device_; }
  void SetDevice(NodeRef device);

  const scada::RelativeTimeRange& time_range() const SCADA_LIFETIME_BOUND {
    return time_range_;
  }
  void SetTimeRange(const scada::RelativeTimeRange& time_range);

  WatchMode mode() const { return mode_; }
  void SetMode(WatchMode mode);

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

  // Indices into `events_` that the current mode shows.
  std::vector<int> visible_;

  scada::RelativeTimeRange time_range_{std::chrono::minutes(15)};

  // Sorted by `scada::Event::time`.
  std::vector<Row> events_;

  bool paused_ = false;
};
