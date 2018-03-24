#pragma once

#include <deque>
#include <memory>

#include "base/files/file_path.h"
#include "core/event.h"
#include "ui/base/models/table_model.h"

namespace scada {
class MonitoredItem;
class MonitoredItemService;
}  // namespace scada

class NodeService;

struct WatchModelContext {
  NodeService& node_service_;
  scada::MonitoredItemService& monitored_item_service_;
};

class WatchModel : private WatchModelContext, public ui::TableModel {
 public:
  explicit WatchModel(WatchModelContext&& context);

  const scada::NodeId& device_id() const { return device_id_; }
  void SetDeviceID(scada::NodeId device_id);

  bool paused() const { return paused_; }
  void set_paused(bool paused) { paused_ = paused; }

  void Clear();

  void SaveLog(const base::FilePath& path);

  // ui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;

 private:
  void OnEvent(const scada::Status& status, const scada::Event& event);
  void AddLine(const scada::Event& event);

  typedef std::deque<scada::Event> Events;
  Events events_;

  scada::NodeId device_id_;
  std::unique_ptr<scada::MonitoredItem> monitored_item_;

  bool paused_ = false;
};
