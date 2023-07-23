#pragma once

#include "common/node_state.h"
#include "aui/handlers.h"
#include "aui/os_exchange_data.h"

class ItemDragData {
 public:
  ItemDragData() {}
  explicit ItemDragData(const scada::NodeId& item_id) : node_id_(item_id) {}

  const scada::NodeId& item_id() const { return node_id_; }

  void Save(base::Pickle& pickle) const;
  bool Load(const base::Pickle& pickle);

  void Save(aui::OSExchangeData& data) const;
  bool Load(const aui::OSExchangeData& data);

  void Save(DragData& drag_data) const;
  bool Load(const DragData& drag_data);

  static aui::OSExchangeData::CustomFormat GetCustomFormat();

  inline static const std::string_view kMimeType =
      "application/telecontrol.scada.nodes";

 private:
  scada::NodeId node_id_;
};
