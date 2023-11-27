#pragma once

#include "aui/models/table_model.h"
#include "base/time_range.h"
#include "components/watch/watch_event_source.h"
#include "node_service/node_ref.h"
#include "scada/event.h"
#include "timed_data/timed_data_view.h"

#include <filesystem>
#include <memory>
#include <vector>

class NodeService;

struct WatchModelContext {
  NodeService& node_service_;
  WatchEventSource& event_source_;
};

class WatchModel : private WatchModelContext,
                   public aui::TableModel,
                   protected WatchEventSource::Delegate {
 public:
  explicit WatchModel(WatchModelContext&& context);

  const NodeRef& device() const { return device_; }
  void SetDevice(NodeRef device);

  const TimeRange& time_range() const { return time_range_; }
  void SetTimeRange(const TimeRange& time_range);

  bool paused() const { return paused_; }
  void set_paused(bool paused) { paused_ = paused; }

  void Clear();

  void SaveLog(const std::filesystem::path& path);

  // ui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(aui::TableCell& cell) override;

 protected:
  // WatchEventSource
  virtual void OnEvent(const scada::Event& event) override;
  virtual void OnError(const scada::Status& status) override;

 private:
  void AddLine(const scada::Event& event);

  NodeRef device_;

  TimeRange time_range_ = base::TimeDelta::FromMinutes(15);

  // Sorted by `scada::Event::time`.
  std::vector<scada::Event> events_;

  bool paused_ = false;
};
