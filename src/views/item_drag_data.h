#pragma once

#include "core/configuration_types.h"
#include "ui/base/dragdrop/os_exchange_data.h"

class ItemDragData {
 public:
  ItemDragData() {}
  explicit ItemDragData(const scada::NodeId& item_id) : item_id_(item_id) {}

  const scada::NodeId& item_id() const { return item_id_; }

  void Save(ui::OSExchangeData& data) const;
  bool Load(const ui::OSExchangeData& data);

  static ui::OSExchangeData::CustomFormat GetCustomFormat();

 private:
  scada::NodeId item_id_;
};
