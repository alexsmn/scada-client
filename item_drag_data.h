#pragma once

#include "controls/handlers.h"
#include "core/configuration_types.h"
#include "ui/base/dragdrop/os_exchange_data.h"

class ItemDragData {
 public:
  ItemDragData() {}
  explicit ItemDragData(const scada::NodeId& item_id) : node_id_(item_id) {}

  const scada::NodeId& item_id() const { return node_id_; }

  void Save(ui::OSExchangeData& data) const;
  bool Load(const ui::OSExchangeData& data);

  void Save(DragData& drag_data) const;

  static ui::OSExchangeData::CustomFormat GetCustomFormat();

  inline static const std::string_view kMimeType =
      "application/telecontrol.scada.nodes";

 private:
  void SaveToPickle(base::Pickle& pickle) const;

  scada::NodeId node_id_;
};
