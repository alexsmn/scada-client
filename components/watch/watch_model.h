#pragma once

#include "node_service/node_ref.h"
#include "core/event.h"
#include "ui/base/models/table_model.h"

#include <deque>
#include <filesystem>
#include <memory>

namespace scada {
class MonitoredItem;
}  // namespace scada

class NodeService;

struct WatchModelContext {
  NodeService& node_service_;
};

class WatchModel : private WatchModelContext, public ui::TableModel {
 public:
  explicit WatchModel(WatchModelContext&& context);

  const NodeRef& device() const { return device_; }
  void SetDevice(NodeRef device);

  bool paused() const { return paused_; }
  void set_paused(bool paused) { paused_ = paused; }

  void Clear();

  void SaveLog(const std::filesystem::path& path);

  // ui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;

 private:
  void OnEvent(const scada::Status& status, const scada::Event& event);
  void AddLine(const scada::Event& event);

  typedef std::deque<scada::Event> Events;
  Events events_;

  NodeRef device_;
  std::shared_ptr<scada::MonitoredItem> monitored_item_;

  bool paused_ = false;
};
