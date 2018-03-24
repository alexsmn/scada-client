#pragma once

#include <deque>
#include <memory>

#include "base/files/file_path.h"
#include "core/event.h"
#include "ui/base/models/table_model.h"
#include "common/node_ref.h"

namespace scada {
class MonitoredItem;
class MonitoredItemService;
}

class WatchModel : public ui::TableModel {
 public:
  explicit WatchModel(scada::MonitoredItemService& monitored_item_service);

  const NodeRef& device() const { return device_; }
  void SetDevice(NodeRef device);

  bool paused() const { return paused_; }
  void set_paused(bool paused) { paused_ = paused; }

  void Clear();

  void SaveLog(const base::FilePath& path);

  // ui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;

 private:
  void OnEvent(const scada::Event& event);

  scada::MonitoredItemService& monitored_item_service_;

  typedef std::deque<scada::Event> Events;
  Events events_;

  NodeRef device_;
  std::unique_ptr<scada::MonitoredItem> monitored_item_;

  bool paused_ = false;
};
